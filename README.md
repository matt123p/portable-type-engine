Portable Type Engine v1.00

Release v1.00 22th April 2015

Please visit the main project's home page here: 
    [https://](https://github.com/matt123p/portable-type-engine)

Please the LICENSE file for details of licensing.


This project is an elegant font rendering engine written in pure C that is design to work well in 
simple embedded systems, such as those that use an ESP32 or a small ARM processor.

It has the following features:

1. Very small compact C code, with a single dependency of stdlib
2. Can render the font at any size at run time from a single font definition
3. High quality font output - Characters are rendered to sub-pixel placement and with full anti-aliasing
4. A simple python tool is included to convert any TrueType or OpenType font to a C for inclusion in your project
5. Compact font definitions, each font is compressed using run-length encoding.

How to use:

1. Convert a font file to a C file (or use the Roboto.c which is included in the test project)
2. Include the font file and the rendering engine (pte.c) in your project
3. Write a single function, "hw_blendPixel" which is called repeatedly by the font engine to draw the font.  See the test project for an example on how to do this.
4. Call pte_drawText(), pte_drawTextRect() or pte_measureText() to render text on to your display

