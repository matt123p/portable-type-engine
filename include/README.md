# Portable Type Engine C API

Include `pte.h` in application code and compile `pte.c` with the generated font
source. All text arguments are UTF-8. A `size` of `(size_t)-1` reads through the
null terminator; any other value is the maximum number of input bytes to read.

Applications must provide this hardware callback:

```c
void hw_blendPixel(int x, int y, int alpha, int color);
```

PTE calls it for every covered destination pixel. `alpha` ranges from `0`
(transparent) to `256` (opaque); `color` is passed through unchanged from the
drawing function.

## Fonts

### `pte_getFont`

```c
pte_font pte_getFont(const pte_base_font *font, int size);
```

Creates a scaled font instance from a generated base font.

- `font`: Unscaled generated font definition.
- `size`: Desired font height in pixels.
- Returns: A font instance for the drawing and measurement functions.

## Measurement

### `pte_measureText`

```c
void pte_measureText(pte_font *font, const char *text, size_t size,
                     int *width, int *height);
```

Measures UTF-8 text using the selected font.

- `font`: Font returned by `pte_getFont`.
- `text`: UTF-8 text to measure.
- `size`: Number of input bytes, or `(size_t)-1` for null-terminated text.
- `width`: Receives the scaled text width in pixels.
- `height`: Receives the scaled line height in pixels.

## Drawing

Rotation arguments accept `0`, `90`, `180`, or `270`. Other values behave as
zero degrees.

### `pte_drawText`

```c
int pte_drawText(pte_font *font, int x, int y, int rotation,
                 const char *text, size_t size, int color);
```

Draws UTF-8 text starting at `x` and baseline `y`.

- `font`: Font returned by `pte_getFont`.
- `x`, `y`: Starting position in pixels; `y` is the baseline.
- `rotation`: Text rotation in degrees.
- `text`: UTF-8 text to draw.
- `size`: Number of input bytes, or `(size_t)-1` for null-terminated text.
- `color`: Value passed to `hw_blendPixel`.
- Returns: The coordinate immediately after the rendered text on its primary axis.

### `pte_drawTextRect`

```c
void pte_drawTextRect(pte_Placement placement, pte_font *font,
                      int x1, int y1, int x2, int y2, int rotation,
                      const char *text, size_t size, int color);
```

Positions text within a rectangle. It does not wrap or clip the text.

- `placement`: Combination of one vertical and one horizontal placement value.
- `font`: Font returned by `pte_getFont`.
- `x1`, `y1`, `x2`, `y2`: Rectangle coordinates.
- `rotation`: Text rotation in degrees.
- `text`: UTF-8 text to draw.
- `size`: Number of input bytes, or `(size_t)-1` for null-terminated text.
- `color`: Value passed to `hw_blendPixel`.

Placement values are:

- Vertical: `TEXT_VCENTER`, `TEXT_LEFT`, `TEXT_RIGHT`.
- Horizontal: `TEXT_HCENTER`, `TEXT_TOP`, `TEXT_BOTTOM`.
- `TEXT_CENTER` combines centered vertical and horizontal placement.

### `pte_drawTextRectWrapped`

```c
void pte_drawTextRectWrapped(pte_Placement placement, pte_font *font,
                             int x1, int y1, int x2, int y2, int rotation,
                             const char *text, size_t size, int color);
```

Wraps text to fit the rectangle. Content that does not fit vertically is not
drawn. Parameters have the same meanings as `pte_drawTextRect`.

## Hardware Callback

### `hw_blendPixel`

```c
void hw_blendPixel(int x, int y, int alpha, int color);
```

PTE does not provide this function. The application must blend `color` into the
display at (`x`, `y`) using `alpha`. The representation of `color` is entirely
application-defined.

For example, an application using a three-byte RGB framebuffer could implement
the callback as follows:

```c
void hw_blendPixel(int x, int y, int alpha, int color)
{
    if (x < 0 || x >= g_width || y < 0 || y >= g_height)
        return;

    int index = (y * g_width + x) * 3;
    unsigned char *pixel = &g_imageData[index];
    int inverse_alpha = 256 - alpha;
    int red = (color >> 16) & 0xff;
    int green = (color >> 8) & 0xff;
    int blue = color & 0xff;

    pixel[0] = ((pixel[0] * inverse_alpha) >> 8) + ((red * alpha) >> 8);
    pixel[1] = ((pixel[1] * inverse_alpha) >> 8) + ((green * alpha) >> 8);
    pixel[2] = ((pixel[2] * inverse_alpha) >> 8) + ((blue * alpha) >> 8);
}
```

This blends each incoming glyph pixel into the framebuffer to implement
anti-aliasing. Adapt the bounds, buffer layout, and color extraction to the
target display.
