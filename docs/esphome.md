# ESPHome and LVGL

Portable Type Engine works with stock ESPHome without patching ESPHome or
replacing its LVGL component. The PTE-owned external component creates ordinary
ESPHome font IDs, owns the corresponding `lv_font_t` objects, and bundles
Roboto and Material Icons.

Using a bundled font is YAML-only. **Python, Pillow, FontTools, and local font
files are not required unless you want to build a custom font.**

## External component

The component is under the repository's `src` path, so use the expanded
Git source form:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/matt123p/portable-type-engine.git
      ref: main
      path: src/esphome
    components: [pte_font]
```

For reproducible firmware, replace `main` with a tested release tag or commit
hash.

## Bundled fonts

The available bundled names are:

- `roboto_regular`, which is the default
- `roboto_bold`
- `roboto_italic`
- `roboto_bold_italic`
- `material_icons`

Declare each face once and give it a base ESPHome ID:

```yaml
pte_font:
  - id: ui_font
    font: roboto_regular

  - id: ui_bold
    font: roboto_bold

  - id: ui_icons
    font: material_icons
```

The component creates suffixed IDs for every integer size from 6 through 75%
of the generated font's sample size, rounded down. All bundled fonts are
sampled at 128 pixels, so the declarations above create:

- `ui_font_6` through `ui_font_96`
- `ui_bold_6` through `ui_bold_96`
- `ui_icons_6` through `ui_icons_96`

These IDs are available to ESPHome during configuration validation, but the
component emits C++ font objects only for IDs that are actually referenced.
References from LVGL properties, styles, default fonts, and `id(...)` inside
lambdas are detected. Unused sizes consume no font-object RAM or descriptor
allocation in the firmware. If none of a bundled face's IDs are referenced,
that face's generated C source is excluded from the build as well.

Place the `pte_font:` block before the `lvgl:` block in the YAML file or merged
package order. ESPHome currently validates custom font providers in load order;
placing `lvgl:` first can produce `This option requires component font` when it
encounters a PTE ID.

Use the IDs directly in LVGL configuration:

```yaml
lvgl:
  pages:
    - id: main_page
      widgets:
        - label:
            text: "Portable Type Engine"
            text_font: ui_font_20

        - label:
            text: "One bundled font, several sizes"
            text_font: ui_font_14
```

Each used ID has a lightweight LVGL font object. All used sizes of the same
face share its bundled glyph data in flash.

## Requirements

For the bundled fonts, the only font-specific requirement is:

- ESPHome with LVGL 9.4 or newer

The font data is already generated and compiled into the component. The ESP
device does not parse TTF or OTF files, and the development machine does not
need Python or font-conversion packages.

The following are required **only when generating a custom font**:

- Python 3
- Pillow and FontTools
- A TrueType or OpenType source font

Install the optional converter dependencies with:

```sh
python -m pip install Pillow fonttools
```

## Automatic font generation

The component can generate fonts from TTF/OTF files automatically during
ESPHome configuration validation. This eliminates the need to run the converter
manually.

Specify a `font_file` with optional character selection:

```yaml
pte_font:
  - id: ui_custom
    font_file: fonts/MyFont.ttf
    ranges:
      - "0x20-0x7e"    # ASCII printable
      - "0xa0-0xff"    # Latin-1 supplement
    symbols: "€£→✓"    # Additional symbols
```

Available options:

- `font_file`: Path to a TTF/OTF font file (required)
- `ranges`: List of Unicode ranges like `"0x20-0x7e,0xa0-0xff"` or multiple strings
- `symbols`: Individual characters to include (e.g., `"€£→✓"`)
- `all_glyphs`: Include every glyph mapped by the font (for icon fonts)
- `font_axes`: Variable font axes as a map (e.g., `wght: 700`)
- `generated_name`: Override the C symbol name (must be a valid C identifier)

The component generates a C file in the `.esphome` build directory and
creates suffixed IDs starting at size 6. The generated font is sampled at 128
pixels.

For variable fonts, set axes:

```yaml
pte_font:
  - id: ui_bold
    font_file: fonts/Roboto-Variable.ttf
    font_axes:
      wght: 700    # Weight
    ranges:
      - "0x20-0x7e"
```

For icon fonts, include all glyphs:

```yaml
pte_font:
  - id: ui_icons
    font_file: icons/MaterialIcons.ttf
    all_glyphs: true
```

## Custom fonts (pre-generated)

Skip this section when using automatic generation above.

To build a custom typeface or icon font manually, run the converter on the
development machine. This example includes the ISO 8859-1 graphic characters:

```sh
python src/font-tool/fontsampler.py \
  --font /path/to/MyFont.ttf \
  --output /path/to/esphome/fonts/my_font_pte.c \
  --range "0x20-0x7e,0xa0-0xff" \
  --name My_Font
```

PowerShell example:

```powershell
python src/font-tool/fontsampler.py `
  --font C:\path\to\MyFont.ttf `
  --output C:\path\to\esphome\fonts\my_font_pte.c `
  --range "0x20-0x7e,0xa0-0xff" `
  --name My_Font
```

Use `--symbols` to add individual characters or `--all` to include every
Unicode code point mapped by an icon font. The generated file records its full
recreation command in its header.

Select the generated source with `file` and its exported getter with `source`:

```yaml
pte_font:
  - id: ui_custom
    file: fonts/my_font_pte.c
    source: get_My_Font
```

The source is the getter name without parentheses. The component reads the
`Font Pixel Height Sampled` value written in the generated file's header and
creates `ui_custom_6` through 75% of that value. It copies and compiles the C
file and supplies the required PTE headers; no manual include entry or
declaration header is needed.

For a hand-written PTE source without the converter header, specify its sample
size explicitly:

```yaml
pte_font:
  - id: ui_custom
    file: fonts/my_font_pte.c
    source: get_My_Font
    source_size: 128
```

Do not reference a bundled C file through `file`; select it using its bundled
`font` name. The component rejects this configuration to prevent duplicate
getter definitions.

## Why the component is needed

ESPHome's `font:` component generates fixed-size bitmap fonts. Its LVGL YAML
validator accepts ESPHome font IDs and LVGL's compiled-in fonts, but it does not
accept an arbitrary `lv_font_t *` as a `text_font` value. The `pte_font`
component provides compatible IDs while retaining PTE's runtime scaling.

## Updating ESPHome or PTE

PTE uses LVGL's public font callbacks and does not patch LVGL. Compile and test
the firmware after changing either ESPHome's LVGL version or the PTE revision.

When following PTE `main`, clean the ESPHome build if PlatformIO continues to
use an older cached checkout:

```sh
esphome clean display.yaml
esphome compile display.yaml
```

For production devices, pin PTE to a tested tag or commit instead of `main`.

## Troubleshooting

### `This option requires component font`

Place the `pte_font:` block before `lvgl:` in the YAML file and in merged
package order so ESPHome registers the font provider before validating LVGL.

### A custom getter is not found

Check the getter at the end of the generated C file and use that exact name in
`source`, without parentheses. Confirm that `file` is relative to the ESPHome
configuration directory and points to the generated C file.

### Text has missing characters

Bundled Roboto contains the ISO 8859-1 graphic characters. For other scripts or
custom icon sets, regenerate a custom font with the required `--range`,
`--symbols`, or `--all` selection. PTE cannot render a glyph that is absent from
the generated font data.

### `lv_pte.h` or `pte.h` is not found

Confirm that the external component source and `path: src/esphome` are correct,
then run `esphome clean display.yaml` so PlatformIO refreshes the checkout.

## Scope

This integration affects LVGL only. ESPHome components that render through
`display:` continue to use ESPHome's normal bitmap fonts.
