# LVGL examples

These Arduino-compatible sketches demonstrate the Portable Type Engine add-on
for LVGL 9.4 or newer.

Before running either example, install both the **LVGL** and **Portable Type
Engine** libraries, then add your board's LVGL display-driver initialization at
the marked location in `setup()`. Display setup is hardware-specific and is not
included here.

## Multiple sizes

[`LvglMultipleSizes`](LvglMultipleSizes/LvglMultipleSizes.ino) creates three
`lv_font_t` objects at different pixel sizes from one compact PTE font source.
This demonstrates how one generated font can replace several fixed-size bitmap
fonts.

## Runtime resize

[`LvglRuntimeResize`](LvglRuntimeResize/LvglRuntimeResize.ino) uses a slider to
resize an existing PTE font. After calling `lv_pte_set_size()`, it reports the
style change to LVGL so the label is measured and rendered again.

## Using your own font

Convert a TTF or OTF font using the tool in [`src/font-tool`](../src/font-tool/README.md),
add the generated C file to your sketch or project, and replace
`get_pte_demo_font()` with the generated font function:

```c
pte_base_font * get_MyFont(void);

lv_font_t * font = lv_pte_create(get_MyFont(), 28);
lv_obj_set_style_text_font(label, font, 0);
```

See the [LVGL add-on guide](../docs/lvgl.md) for PlatformIO and Arduino
installation instructions.
