import json
import re
from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import font
from esphome.const import CONF_FILE, CONF_FONT, CONF_ID, CONF_SIZE, CONF_SOURCE
from esphome.core import CORE
from esphome.core.config import add_includes

MULTI_CONF = True

pte_font_ns = cg.esphome_ns.namespace("pte_font")
PteFont = pte_font_ns.class_("PteFont", font.Font, cg.Component)

_COMPONENT_DIR = Path(__file__).parent
_FONT_REGISTRY = json.loads(
    (_COMPONENT_DIR / "fonts.json").read_text(encoding="utf-8")
)["fonts"]
_FONT_NAMES = tuple(_FONT_REGISTRY)
_BUNDLED_SOURCES = {font["source"] for font in _FONT_REGISTRY.values()}
_ADDED_CUSTOM_FILES: set[str] = set()
_DECLARED_CUSTOM_SOURCES: set[str] = set()


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
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PteFont),
            cv.Optional(CONF_FONT): cv.one_of(*_FONT_NAMES, lower=True),
            cv.Optional(CONF_FILE): _validate_c_file,
            cv.Optional(CONF_SOURCE): _validate_source,
            cv.Required(CONF_SIZE): cv.int_range(min=1, max=512),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_config,
)


async def to_code(config):
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
        source = _FONT_REGISTRY[config[CONF_FONT]]["source"]

    var = cg.new_Pvariable(
        config[CONF_ID], cg.RawExpression(f"{source}()"), config[CONF_SIZE]
    )
    await cg.register_component(var, config)
