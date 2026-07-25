# Font tool

`fontsampler.py` converts a TrueType or OpenType font into a C source file for
Portable Type Engine (PTE). The output contains glyph metrics, compressed
bitmaps, kerning tables, and a getter for the generated `pte_base_font`.

## Generated format

Glyphs are ordered by Unicode code point for binary lookup. The scanline
renderer allocates its accumulator for each glyph, so source glyphs are not
limited to 128 pixels wide.

### Bitmaps

Each glyph begins with one repeat-previous bit per scanline, padded to a byte
boundary. A repeated row has no pixel payload. Literal rows form one continuous
stream, allowing runs to cross scanline boundaries.

Alternating background and foreground runs use
[Golomb–Rice coding](https://en.wikipedia.org/wiki/Golomb_coding#Rice_coding)
with `k=4`: a unary quotient followed by a four-bit remainder. The first run is
background and may be empty.

### Kerning

The tool reads both legacy `kern` tables and GPOS pair positioning, including
class-based kerning. It scales adjustments to the sampled size, removes values
that round to zero, sorts pairs for binary lookup, and shares identical
first-glyph rows.

Compact kerning is selected when:

- referenced second-glyph indices fit in eight bits;
- there are no more than 255 shared rows;
- there are no more than 65,535 entries; and
- sampled adjustments fit in a signed byte.

The glyph table itself may contain more than 256 glyphs. Fonts that exceed a
compact limit use the wide 16-bit format.

## Installation

Install Python 3 and the two converter dependencies:

```sh
python -m pip install Pillow fonttools
```

## Usage

```sh
python fontsampler.py --font <font-file> --output <output.c> [options]
```

Options:

- `--font`: input TTF or OTF file.
- `--output`: generated C file.
- `--range`: decimal or `0x` Unicode ranges; repeat the option as needed.
- `--symbols`: individual Unicode characters to include.
- `--all`: include every Unicode code point mapped by the font.
- `--axis TAG=VALUE`: set a variable-font axis; repeat as needed.
- `--name C_NAME`: set the generated C symbol and getter name.

Without `--range`, `--symbols`, or `--all`, the default set is
`U+0020–U+007E` and `U+00A0–U+00FF`. Using `--symbols` alone includes only
those symbols. Overlapping selections are deduplicated.

### Examples

Basic conversion:

```sh
python fontsampler.py --font MyFont.ttf --output my_font.c
```

Selected ranges and symbols:

```sh
python fontsampler.py --font MyFont.ttf --output my_font.c \
  --range "0x20-0x7e,0xa0-0xff" --symbols "€£→✓"
```

Variable font:

```sh
python fontsampler.py --font Roboto-Variable.ttf --output roboto_bold.c \
  --axis wght=700 --name Roboto_Bold
```

Complete icon font:

```sh
python fontsampler.py --font MyIcons.ttf --output my_icons.c \
  --all --name My_Icons
```

PTE text is UTF-8. Drawing and measurement functions take a byte count; pass
`(size_t)-1` for a null-terminated string.

The generated source works with the
[LVGL adapter](../../docs/lvgl.md) and the
[ESPHome component](../../docs/esphome.md) without another conversion step.
