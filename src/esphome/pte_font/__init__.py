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

pte_font_ns = cg.esphome_ns.namespace("pte_font_component")
PteFont = pte_font_ns.class_("PteFont", font.Font)

CONF_SOURCE_SIZE = "source_size"
CONF_SIZE_IDS = "_size_ids"
CONF_FONT_FILE = "font_file"
CONF_RANGES = "ranges"
CONF_SYMBOLS = "symbols"
CONF_ALL_GLYPHS = "all_glyphs"
CONF_FONT_AXES = "font_axes"
CONF_GENERATED_NAME = "generated_name"
MIN_FONT_SIZE = 6
MAX_SOURCE_SIZE = 1024
DOMAIN = "pte_font"
_SAMPLE_SIZE = 128

_COMPONENT_DIR = Path(__file__).parent
_FONT_REGISTRY = json.loads(
    (_COMPONENT_DIR / "fonts.json").read_text(encoding="utf-8")
)["fonts"]
_FONT_NAMES = tuple(_FONT_REGISTRY)
_BUNDLED_SOURCES = {font["source"] for font in _FONT_REGISTRY.values()}
_ADDED_CUSTOM_FILES: set[str] = set()
_DECLARED_CUSTOM_SOURCES: set[str] = set()
_GENERATED_SOURCES: dict[str, str] = {}  # Maps generated C name to source file
_ENGINE_FILES_ADDED = False  # Track if engine source files have been added


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
        excluded.append("pte_font.cpp")
    # Exclude generated fonts directory from being copied to build
    excluded.append("build/generated_fonts")
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


def _generate_font_file(config, id_prefix):
    """Generate a C font file from a TTF/OTF font file."""
    try:
        from PIL import ImageFont
        from fontTools.ttLib import TTFont
        from fontTools.varLib.instancer import instantiateVariableFont
        from io import BytesIO
    except ImportError:
        raise cv.Invalid(
            "font_file requires PIL, fontTools. "
            "Install with: pip install Pillow fontTools"
        )

    font_file = CORE.relative_config_path(config[CONF_FONT_FILE])
    if not font_file.exists():
        raise cv.Invalid(f"font file not found: {font_file}")

    # Import FontSampler from the font_tool
    # When checking out src/, font-tool is at src/font-tool
    font_tool_dir = _COMPONENT_DIR.parent.parent / "font-tool"
    if not font_tool_dir.exists():
        raise cv.Invalid(
            f"font_tool directory not found at {font_tool_dir}"
        )

    import sys
    if str(font_tool_dir) not in sys.path:
        sys.path.insert(0, str(font_tool_dir))

    try:
        from fontsampler import FontSampler, parse_ranges
    except ImportError:
        raise cv.Invalid("Could not import FontSampler from font_tool")

    # Get font parameters
    unicode_ranges = None
    if CONF_RANGES in config:
        unicode_ranges = []
        for range_str in config[CONF_RANGES]:
            try:
                unicode_ranges.extend(parse_ranges(range_str))
            except Exception as e:
                raise cv.Invalid(f"invalid range '{range_str}': {e}")

    symbols = config.get(CONF_SYMBOLS, "")
    all_glyphs = config.get(CONF_ALL_GLYPHS, False)
    font_name = config.get(CONF_GENERATED_NAME)
    axes = config.get(CONF_FONT_AXES, {})

    # Load the font
    try:
        tt_font = TTFont(str(font_file))
    except Exception as e:
        raise cv.Invalid(f"Unable to load font: {e}")

    # Handle variable font axes
    if axes:
        try:
            tt_font = instantiateVariableFont(
                tt_font, axes, inplace=False, updateFontNames=True
            )
            font_data = BytesIO()
            tt_font.save(font_data)
            font_data.seek(0)
            sample_font = ImageFont.truetype(font_data, _SAMPLE_SIZE)
        except Exception as e:
            raise cv.Invalid(f"Unable to instantiate variable font: {e}")
    else:
        try:
            sample_font = ImageFont.truetype(str(font_file), _SAMPLE_SIZE)
        except Exception as e:
            raise cv.Invalid(f"Unable to load font: {e}")

    # Determine the generated C function name
    # Use the id prefix to avoid collisions between different font entries
    if font_name:
        c_name = font_name
    else:
        # Get font family name and sanitize for C identifier
        font_family = sample_font.getname()[0].replace(" ", "_")
        c_name = f"generated_{font_family}"

    # Add id prefix to avoid collisions
    prefixed_c_name = f"{id_prefix}_{c_name}"

    # Generate the font
    sampler = FontSampler(
        unicode_ranges=unicode_ranges,
        symbols=symbols,
        all_glyphs=all_glyphs,
        font_name=prefixed_c_name,
    )

    # Write to a generated subdirectory to avoid PlatformIO picking it up
    build_dir = Path(CORE.relative_config_path(".esphome/build/generated_fonts"))
    build_dir.mkdir(parents=True, exist_ok=True)

    output_file = build_dir / f"{prefixed_c_name}.c"

    try:
        sampler.convertFont(sample_font, tt_font, str(output_file))
    except Exception as e:
        raise cv.Invalid(f"Font generation failed: {e}")

    # Return relative path from config directory for add_includes
    return f".esphome/build/generated_fonts/{prefixed_c_name}.c", f"get_{prefixed_c_name}"


def _validate_config(config):
    # ESPHome's LVGL validator accepts IDs typed as font::Font only when the
    # font integration is marked as loaded. PteFont supplies the same LVGL
    # contract without loading ESPHome's bitmap font generator.
    CORE.loaded_integrations.add("font")

    has_file = CONF_FILE in config
    has_source = CONF_SOURCE in config
    has_font = CONF_FONT in config
    has_font_file = CONF_FONT_FILE in config

    # Check for conflicting options
    options_count = sum([has_file, has_font, has_font_file])
    if options_count > 1:
        raise cv.Invalid(
            "Only one of font, file+source, or font_file may be specified"
        )

    if has_file != has_source:
        raise cv.Invalid("custom fonts require both file and source")
    if has_file and has_font:
        raise cv.Invalid("font cannot be combined with file and source")
    if has_file and config[CONF_SOURCE] in _BUNDLED_SOURCES:
        raise cv.Invalid(
            "source is already bundled; select it with the font option instead"
        )

    # Handle font_file option - generate the font
    if has_font_file:
        # Validate font_file options
        if CONF_SOURCE_SIZE in config:
            raise cv.Invalid("source_size cannot be used with font_file")
        if CONF_SOURCE in config:
            raise cv.Invalid("source cannot be used with font_file")

        # Generate the font file with id prefix to avoid name collisions
        generated_file, source_name = _generate_font_file(config, config[CONF_ID])

        # Store generated file info
        config[CONF_FILE] = generated_file
        config[CONF_SOURCE] = source_name
        config[CONF_SOURCE_SIZE] = _SAMPLE_SIZE

    # Default to roboto_regular if no font specified
    if not has_file and not has_font and not has_font_file:
        config[CONF_FONT] = "roboto_regular"

    # Determine source size
    if CONF_FILE in config:
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


def _validate_axes_dict(value):
    """Convert and validate axes dict to key=value pairs."""
    if not isinstance(value, dict):
        raise cv.Invalid("axes must be a dictionary")
    result = {}
    for key, val in value.items():
        if len(key) != 4:
            raise cv.Invalid(f"axis tag must be 4 characters: {key}")
        try:
            result[key] = float(val)
        except (ValueError, TypeError):
            raise cv.Invalid(f"axis value must be numeric: {val}")
    return result


def _validate_c_identifier(value):
    """Validate that a string is a valid C identifier."""
    if not re.fullmatch(r"^[A-Za-z_][A-Za-z0-9_]*$", value):
        raise cv.Invalid("must be a valid C identifier (letters, digits, underscores)")
    return value


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
            # Font file generation options
            cv.Optional(CONF_FONT_FILE): cv.file_,
            cv.Optional(CONF_RANGES): cv.ensure_list(cv.string_strict),
            cv.Optional(CONF_SYMBOLS): cv.string_strict,
            cv.Optional(CONF_ALL_GLYPHS): cv.boolean,
            cv.Optional(CONF_FONT_AXES): _validate_axes_dict,
            cv.Optional(CONF_GENERATED_NAME): cv.All(
                cv.string_strict,
                _validate_c_identifier,
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

    # Add engine source files only once
    global _ENGINE_FILES_ADDED
    if not _ENGINE_FILES_ADDED:
        # Get paths relative to project directory
        # The external components are checked out to .esphome/external_components/{hash}/src/
        # We need to find that src directory and access lvgl/ and pte/ from there
        try:
            # Start from component dir and find the src directory
            current = _COMPONENT_DIR
            src_dir = None
            while current.parent != Path(current.anchor):
                if current.name == "src":
                    src_dir = current
                    break
                current = current.parent

            if src_dir is None:
                raise cv.Invalid("Could not find src directory in component checkout")

            # From src/ directory, access lvgl/ and pte/
            lv_pte_c = src_dir / "lvgl" / "lv_pte.c"
            pte_c = src_dir / "pte" / "pte.c"

            if not lv_pte_c.exists():
                raise cv.Invalid(f"lv_pte.c not found at {lv_pte_c}")
            if not pte_c.exists():
                raise cv.Invalid(f"pte.c not found at {pte_c}")

            CORE.add_job(add_includes, [str(lv_pte_c)], False)
            CORE.add_job(add_includes, [str(pte_c)], False)
            _ENGINE_FILES_ADDED = True
        except Exception as e:
            raise cv.Invalid(f"Failed to add engine source files: {e}")

    cg.add_define("USE_FONT")
    # Add include path for the pte_font component directory
    cg.add_build_flag(f'-I"{_COMPONENT_DIR}"')
    # Add include path for the PTE engine (src/pte)
    pte_include = (_COMPONENT_DIR.parent.parent / "pte").as_posix()
    cg.add_build_flag(f'-I"{pte_include}"')
    # Add include path for LVGL PTE adapter (src/lvgl)
    lvgl_include = (_COMPONENT_DIR.parent.parent / "lvgl").as_posix()
    cg.add_build_flag(f'-I"{lvgl_include}"')
    # USE_FONT makes LVGL's compatibility overloads include font/font.h even
    # when no ESPHome bitmap font instance is configured. In that case provide
    # only the declaration needed by those inline overloads.
    if "font" not in CORE.config:
        compat_include = (_COMPONENT_DIR / "compat").as_posix()
        cg.add_build_flag(f'-I"{compat_include}"')

    if CONF_FILE in config:
        source = config[CONF_SOURCE]
        file = config[CONF_FILE]
        # Normalize the file path for tracking
        file_normalized = str(Path(file).resolve())
        if file_normalized not in _ADDED_CUSTOM_FILES:
            CORE.add_job(add_includes, [file], False)
            _ADDED_CUSTOM_FILES.add(file_normalized)
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
