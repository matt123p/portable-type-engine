import json
import re
from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import font
from esphome.config import iter_ids
from esphome.const import CONF_FILE, CONF_FONT, CONF_ID, CONF_SOURCE
from esphome.core import CORE
from esphome.core.config import add_includes

MULTI_CONF = True

pte_font_ns = cg.esphome_ns.namespace("pte_font")
PteFont = pte_font_ns.class_("PteFont", font.Font)

CONF_SOURCE_SIZE = "source_size"
CONF_SIZE_IDS = "_size_ids"
MIN_FONT_SIZE = 6
MAX_SOURCE_SIZE = 1024
DOMAIN = "pte_font"

_COMPONENT_DIR = Path(__file__).parent
_FONT_REGISTRY = json.loads(
    (_COMPONENT_DIR / "fonts.json").read_text(encoding="utf-8")
)["fonts"]
_FONT_NAMES = tuple(_FONT_REGISTRY)
_BUNDLED_SOURCES = {font["source"] for font in _FONT_REGISTRY.values()}
_ADDED_CUSTOM_FILES: set[str] = set()
_DECLARED_CUSTOM_SOURCES: set[str] = set()


def _get_codegen_data():
    return CORE.data.setdefault(
        DOMAIN, {"runtime_used": False, "bundled_files": set()}
    )


def FILTER_SOURCE_FILES():
    data = _get_codegen_data()
    excluded = [
        font["file"]
        for font in _FONT_REGISTRY.values()
        if font["file"] not in data["bundled_files"]
    ]
    if not data["runtime_used"]:
        excluded.extend(("lv_pte.c", "pte_font.cpp"))
    return excluded


def _validate_source(value):
    value = cv.string_strict(value)
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value) is None:
        raise cv.Invalid("source must be a C function name without parentheses")
    return value


def _validate_c_file(value):
    path = CORE.relative_config_path(cv.file_(value))
    if path.suffix.lower() != ".c":
        raise cv.Invalid("PTE generated font files must use the .c extension")
    return str(path)


def _read_generated_source_size(path):
    with Path(path).open(encoding="utf-8") as source_file:
        header = source_file.read(4096)
    match = re.search(r"Font Pixel Height Sampled:\s*([0-9]+)", header)
    if match is None:
        raise cv.Invalid(
            "generated font header has no sample size; set source_size explicitly"
        )
    return cv.int_range(min=1, max=MAX_SOURCE_SIZE)(match.group(1))


def _validate_config(config):
    # ESPHome's LVGL validator accepts IDs typed as font::Font only when the
    # font integration is marked as loaded. PteFont supplies the same LVGL
    # contract without loading ESPHome's bitmap font generator.
    CORE.loaded_integrations.add("font")

    has_file = CONF_FILE in config
    has_source = CONF_SOURCE in config
    has_font = CONF_FONT in config

    if has_file != has_source:
        raise cv.Invalid("custom fonts require both file and source")
    if has_file and has_font:
        raise cv.Invalid("font cannot be combined with file and source")
    if has_file and config[CONF_SOURCE] in _BUNDLED_SOURCES:
        raise cv.Invalid(
            "source is already bundled; select it with the font option instead"
        )
    if not has_file and not has_font:
        config[CONF_FONT] = "roboto_regular"

    if has_file:
        source_size = config.get(CONF_SOURCE_SIZE)
        if source_size is None:
            source_size = _read_generated_source_size(config[CONF_FILE])
    else:
        if CONF_SOURCE_SIZE in config:
            raise cv.Invalid("source_size is only valid with a custom file")
        source_size = _FONT_REGISTRY[config[CONF_FONT]]["source_size"]

    max_size = source_size * 3 // 4
    if max_size < MIN_FONT_SIZE:
        raise cv.Invalid(
            f"font sample size {source_size} is too small; size {MIN_FONT_SIZE} cannot be generated"
        )

    base_id = config[CONF_ID]
    config[CONF_SOURCE_SIZE] = source_size
    config[CONF_SIZE_IDS] = {
        size: cv.declare_id(PteFont)(f"{base_id}_{size}")
        for size in range(MIN_FONT_SIZE, max_size + 1)
    }
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.All(cv.string_strict, cv.validate_id_name),
            cv.Optional(CONF_FONT): cv.one_of(*_FONT_NAMES, lower=True),
            cv.Optional(CONF_FILE): _validate_c_file,
            cv.Optional(CONF_SOURCE): _validate_source,
            cv.Optional(CONF_SOURCE_SIZE): cv.int_range(
                min=1, max=MAX_SOURCE_SIZE
            ),
        }
    ),
    _validate_config,
)


async def to_code(config):
    referenced_ids = {
        id_.id
        for id_, _ in iter_ids(CORE.config)
        if not id_.is_declaration
    }
    used_sizes = {
        size: size_id
        for size, size_id in config[CONF_SIZE_IDS].items()
        if size_id.id in referenced_ids
    }
    if not used_sizes:
        return

    data = _get_codegen_data()
    data["runtime_used"] = True

    cg.add_define("USE_FONT")
    cg.add_build_flag("-Isrc/esphome/components/pte_font")
    # USE_FONT makes LVGL's compatibility overloads include font/font.h even
    # when no ESPHome bitmap font instance is configured. In that case provide
    # only the declaration needed by those inline overloads.
    if "font" not in CORE.config:
        compat_include = (_COMPONENT_DIR / "compat").as_posix()
        cg.add_build_flag(f'-I"{compat_include}"')

    if CONF_FILE in config:
        source = config[CONF_SOURCE]
        file = config[CONF_FILE]
        if file not in _ADDED_CUSTOM_FILES:
            CORE.add_job(add_includes, [file], False)
            _ADDED_CUSTOM_FILES.add(file)
        if source not in _DECLARED_CUSTOM_SOURCES:
            cg.add_global(
                cg.RawStatement(
                    f'extern "C" pte_base_font *{source}(void);'
                )
            )
            _DECLARED_CUSTOM_SOURCES.add(source)
    else:
        bundled_font = _FONT_REGISTRY[config[CONF_FONT]]
        source = bundled_font["source"]
        data["bundled_files"].add(bundled_font["file"])

    for size, size_id in used_sizes.items():
        cg.new_Pvariable(
            size_id, cg.RawExpression(f"{source}()"), size
        )
