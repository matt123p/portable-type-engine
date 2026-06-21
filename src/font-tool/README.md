# Portable Type Engine Font Tool User Guide

This tool converts font files (such as TTF files) into C arrays that can be used with the Portable Type Engine. It extracts character glyphs, kerning information, and other metrics from the font file and generates C code that you can include in your projects.

## What Does It Do?

- **Generates Image Data:** Converts the glyphs into image data (packed into an array).
- **Processes Kerning:** Extracts kerning information between pairs of characters, scaling it to the correct pixel values.
- **Produces C Code:** Outputs a C file with arrays for glyphs, image data, and kerning data, along with a function to retrieve the font structure.

Generated glyph and kerning tables are sorted for PTE's binary searches. Both
legacy `kern` tables and GPOS pair positioning (including class-based kerning)
are supported. The runtime allocates its scanline accumulator to fit each glyph,
so source glyph widths are not limited to 128 pixels.

## Installation

### Prerequisites
- **Python 3.x:** Ensure Python is installed. You can download it from [python.org](https://www.python.org/downloads/).
- **Required Python Packages:**  
  This tool depends on the following Python packages:
  - [Pillow](https://pillow.readthedocs.io/)
  - [fontTools](https://github.com/fonttools/fonttools)

### Installation Steps

1. **Install Python 3:**  
   Follow the installation instructions on [python.org](https://www.python.org/downloads/).

2. **Install Required Packages:**  

Open a terminal or command prompt and run:
```sh
pip install Pillow fonttools
```

## How to Run the Tool

1.  **Open a Terminal:**  
    Open a command prompt or terminal in the folder that contains fontsampler.py.
2.  **Run the Tool:**  
    Use the following command format:

```sh
python fontsampler.py --font <font-file-path> --output <output-file> [options]
```

-   `--font <font-file-path>`: Required path to the input font file (e.g., `C:\path\to\yourfont.ttf`).
-   `--output <output-file>`: Required path to the output C file.
-   [--range <start-end,...>]: (Optional) Add Unicode ranges using decimal or `0x` values. The option can be repeated.
-   [--symbols <symbols>]: (Optional) Add individual Unicode symbols.

### Examples

**Basic Usage:**

```sh
python fontsampler.py --font C:\path\to\yourfont.ttf --output myfont.c
```
**Selecting Unicode Characters:**

Ranges and individual symbols can be combined. Overlapping selections are
deduplicated. Values may be decimal or `0x`-prefixed Unicode code points, and
`--range` may be repeated:

```sh
python fontsampler.py --font C:\path\to\yourfont.ttf --output myfont.c --range "0x20-0x7e, 0xa0-0xff" --symbols "€£→✓"
```

When neither option is supplied, the tool includes `U+0020-U+007E` and
`U+00A0-U+00FF`. When `--symbols` is used alone, only those symbols are included.

All text passed to PTE is UTF-8. The `size` argument to drawing and measurement
functions is a byte count; use `(size_t)-1` for null-terminated text.

Generated files can also be passed directly to `lv_pte_create()` when using the
[LVGL add-on](../../docs/lvgl.md); no second conversion format is needed.

