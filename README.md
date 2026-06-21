# Portable Type Engine v2.00

Release v2.00 23 March 2025

Please visit the main project's home page here: 
    [GitHub](https://github.com/matt123p/portable-type-engine)

Please the LICENSE file for details of licensing.


## Overview

This project is a font rendering engine written in pure C for low-to-moderate
resolution displays, such as typical 480p or 720p LCD panels, coupled with a
lower-power CPU such as an ESP32 or a small ARM processor.

<img src="images/Full%20UI%20example.png" width="475"
     alt="Example embedded display interface rendered with Portable Type Engine">

- It is specifically designed for greyscale or color displays.  It renders fonts with anti-aliasing 
improving the look of the text, particulary for small text.

- It also re-sizes the font at run-time, meaning you do not need to have multiple copies of the font
for each font size.

- It is very fast, which eliminates the need for a font-cache that other scalable font engines
use.

## Features

It has the following features:

1. Very small compact C code, with a single dependency of stdlib
2. Can render the font at any size at run time from a single font definition
3. High quality font output - Characters are rendered to sub-pixel placement and with full anti-aliasing
4. A simple python tool is included to convert any TrueType or OpenType font to a C for inclusion in your project
5. Compact font definitions, each font is compressed using run-length encoding.
6. Unicode glyph tables with UTF-8 text input.

### Full anti-aliasing

Glyph coverage is blended into the destination pixels to produce smooth edges
and readable text, particularly at small sizes on lower-resolution displays.

<img src="images/example.png" width="1100"
     alt="Example of anti-aliased text rendered by Portable Type Engine">

### Sub-pixel placement

Glyphs are positioned using fractional pixel coordinates rather than being
forced onto whole-pixel boundaries. Anti-aliasing distributes each glyph's
coverage across the neighbouring pixels to compensate for that fractional
position. This preserves the font's exact advances, kerning, and intended text
spacing even when a character does not begin on a pixel boundary.

<img src="images/sub-pixel%20alignment.png" width="747"
     alt="Characters placed on sub-pixel boundaries">


## Use

How to use:

1. Convert a font file to a C file (or use the Roboto.c which is included in the test project)
2. Include the font file and the rendering engine (pte.c) in your project
3. Implement the [`hw_blendPixel`](include/README.md#hardware-callback) callback used by the engine to draw each pixel.
4. Call `pte_drawText()`, `pte_drawTextRect()` or `pte_measureText()` to render text on to your display.


Example usage

``` C
pte_font f = pte_getFont(get_Roboto128(), 40);
y = f.m_baseline;
pte_drawText(&f, 5, y, 0, "Example text", -1, 0);
y += f.m_line_height;
```

## Documentation

- [C API reference](include/README.md)
- [Font conversion tool guide](FontTool/README.md)
