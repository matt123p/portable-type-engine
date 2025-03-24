# Portable Type Engine v2.00

Release v2.00 23 March 2025

Please visit the main project's home page here: 
    [GitHub](https://github.com/matt123p/portable-type-engine)

Please the LICENSE file for details of licensing.


## Overview

This project is an elegant font rendering engine written in pure C that is design to work well in simple embedded systems, such as those that use an ESP32 or a small ARM processor.

## Features

It has the following features:

1. Very small compact C code, with a single dependency of stdlib
2. Can render the font at any size at run time from a single font definition
3. High quality font output - Characters are rendered to sub-pixel placement and with full anti-aliasing
4. A simple python tool is included to convert any TrueType or OpenType font to a C for inclusion in your project
5. Compact font definitions, each font is compressed using run-length encoding.


## Use

How to use:

1. Convert a font file to a C file (or use the Roboto.c which is included in the test project)
2. Include the font file and the rendering engine (pte.c) in your project
3. Write a single function, "hw_blendPixel" which is called repeatedly by the font engine to draw the font.  See the test project for an example on how to do this.
4. Call `pte_drawText()`, `pte_drawTextRect()` or `pte_measureText()` to render text on to your display.


Example usage

``` C
pte_font f = pte_getFont(get_Roboto128(), 40);
y = f.m_baseline;
pte_drawText(&f, 5, y, 0, "Example text", -1, 0);
y += f.m_line_height;
```

You will need to implement the `hw_blendPixel()` function, that will be called
repeatedly by `pte_drawText()`.

Here is an example of how to implement it for a single RGB buffer:

``` C
void hw_blendPixel(int x, int y, int a, int col)
{
	if (x < 0 || x >= g_width || y < 0 || y >= g_height)
		return;
	int index = (y * g_width + x) * 3; // each pixel has 3 bytes (RGB)

	unsigned char p[3];
	p[0] = g_imageData[index];     // Red
	p[1] = g_imageData[index + 1]; // Green
	p[2] = g_imageData[index + 2]; // Blue

	unsigned int c[3];
	c[0] = col & 0xff;         // Blue component in our call order
	c[1] = (col >> 8) & 0xff;  // Green
	c[2] = (col >> 16) & 0xff; // Red

	int b = 256 - a;
	unsigned char newp[3];

	// Blend each channel
	newp[0] = ((p[0] * b) >> 8) + ((c[2] * a) >> 8); // Red
	newp[1] = ((p[1] * b) >> 8) + ((c[1] * a) >> 8); // Green
	newp[2] = ((p[2] * b) >> 8) + ((c[0] * a) >> 8); // Blue

	g_imageData[index] = newp[0];
	g_imageData[index + 1] = newp[1];
	g_imageData[index + 2] = newp[2];
}
```

This function blends the incoming pixel with the existing buffer so that
anti-aliasing is correctly implemented.

## API Reference

### pte_getFont( const pte_base_font* f, int size )

This function creates and returns a scaled font instance that can be used with all other operations of the rendering engine.

**Parameters:**

- `f`  
  A pointer to an unscaled base font definition. This font definition contains the raw glyph information and metrics as provided by the font conversion tool.

- `size`  
  An integer representing the desired height of the text in pixels. The returned font instance will have its metrics adjusted so that the font's height matches this value.

**Return Value:**

Returns a `pte_font` object containing all the necessary data (glyph bitmaps, metrics, baseline, line height, etc.) scaled to the specified size. This object is subsequently passed to functions such as `pte_drawText()` or `pte_measureText()` to render or measure text.

### pte_measureText( pte_font *f, const char *text, int size, int *dx, int *dy )

This function calculates the bounding rectangle for a given string, obtaining its width and height in pixels based on the provided font metrics.

**Parameters:**

- `f`
  The font instance to use for measurement. This should be created using the pte_getFont() function.

- `text`
  The text string to measure.

- `size`
  The number of characters to consider in the string. Pass -1 if the string is null-terminated.

- `dx`
  A pointer to an integer that will receive the width of the string in pixels.

- `dy`
  A pointer to an integer that will receive the height of the string in pixels.

**Return Value:**

The function returns void. The computed width and height are stored in to dx and dy.



### int	pte_drawText( pte_font *font, int x, int y, int r, const char *text, int size, int c )

This function draws a text string onto the display at a specified location using the provided font and color. The text is rendered with anti-aliasing by repeatedly calling the user-defined hw_blendPixel() function.

**Parameters:**

- `font`
  The font to use for rendering. This should be created using the pte_getFont() function.

- `x`
  The x position (in pixels) where the text drawing begins.

- `y`
  The y position (in pixels) corresponding to the font's baseline.

- `r`
  The rotation of the text.  This can be `0`, `90`, `180` or `270` - where `0` is left to right, horizontal text.  All other values will be treated as `0`.

- `text`
  The text string to render.

- `size`
  The number of characters in the string to consider. Pass -1 if the text is null-terminated.

- `c`
  The color to draw the text. This value is passed directly to the hw_blendPixel() function.

**Return Value:**

Returns an integer representing the x coordinate immediately following the drawn text.


### void pte_drawTextRect( pte_Placement o, pte_font *f, int x1, int y1, int x2, int y2, const char *text, int size, int c )

Draw text using the rectangle to position it. This function _does not_ wrap or clip the text to fit the rectangle, it is simply using the rectangle for positioning.

**Parameters:**
- `o`
  The placement within the rectangle to draw the text. See pte_Placement for details.  

  The vertical and horizontal alignment are independent.

  - For vertical alignment it can be: `TEXT_VCENTER`, `TEXT_LEFT` or `TEXT_RIGHT`
  - For horizontal aligment it can be: `TEXT_HCENTER`, `TEXT_TOP`, `TEXT_BOTTOM` 
  - `TEXT_CENTER` is a shortcut for `TEXT_VCENTER | TEXT_HCENTER`


- `f`
  The font to use for rendering. This should be created using the pte_getFont() function.

- `x1`, `y1`, `x2`, `y2`
  The coordinates defining the rectangle within which the text will be placed.

- `r`
  The rotation of the text.  This can be `0`, `90`, `180` or `270` - where `0` is left to right, horizontal text.  All other values will be treated as `0`.

- `text`
The text string to render.

- `size`
The number of characters in the string to consider. Pass -1 if the text is null-terminated.

- `c`
  The color to draw the text. This value is passed directly to the hw_blendPixel() function.

**Return Value:**

This function does not return any value.

### void pte_drawTextRectWrapped( pte_Placement o, pte_font *f, int x1, int y1, int x2, int y2, const char *text, int size, int c )

Draw text using the rectangle to constrain it. This function _does_ wrap the text to fit the rectangle.  If the text doesn't fit, then it is simply not drawn.

**Parameters:**
- `o`
  The placement within the rectangle to draw the text. See pte_Placement for details.  
  
  This function only accepts a vertical alignment parameter.  It can be: `TEXT_VCENTER`, `TEXT_LEFT` or `TEXT_RIGHT`
  

- `f`
  The font to use for rendering. This should be created using the pte_getFont() function.

- `x1`, `y1`, `x2`, `y2`
  The coordinates defining the rectangle within which the text will be placed.

- `r`
  The rotation of the text.  This can be `0`, `90`, `180` or `270` - where `0` is left to right, horizontal text.  All other values will be treated as `0`.

- `text`
The text string to render.

- `size`
The number of characters in the string to consider. Pass -1 if the text is null-terminated.

- `c`
  The color to draw the text. This value is passed directly to the hw_blendPixel() function.

**Return Value:**

This function does not return any value.

## void hw_blendPixel(int x, int y, int a, int col)

This function is _not_ provided, and must be provided by you when you use this library.  It is called repeatedly by the drawing functions to plot pixels on to the display.

**Parameters:**
- `x`
  The x position (in pixels) where to plot the pixel

- `y`
  The y position (in pixels) where to plot the pixel

- `a`
  The opacity of the pixel, where `255` is a opaque and `0` is completely transparent.  This can be considered the amount of grayscale the pixel should have.

- `c`
  The color to draw the pixel. This value is hardware dependent and just passed directly from the text drawing functions.

**Return Value:**

This function is not expected to return any value.
