# LVGL 9 add-on

Portable Type Engine can supply scalable `lv_font_t` objects to an unmodified
LVGL 9 project. The same generated font data works with both the original PTE
drawing API and the LVGL adapter.

## PlatformIO

Add LVGL and this repository to `platformio.ini`:

```ini
[env:your-board]
framework = arduino
lib_deps =
    lvgl/lvgl@^9.4.0
    https://github.com/matt123p/portable-type-engine.git
```

PlatformIO discovers `library.json` and compiles the PTE core and LVGL adapter.
Configure and initialize LVGL as normal for your board and display driver.

## Arduino IDE

Install LVGL using Library Manager. Install this repository using **Sketch >
Include Library > Add .ZIP Library**, or clone it into the Arduino libraries
directory. The examples then appear under **File > Examples > Portable Type
Engine**. Each example leaves display-driver initialization as the one
board-specific step.

## Convert and use a font

Install the converter requirements and generate a C source file:

```sh
python -m pip install -r src/font-tool/requirements.txt
python src/font-tool/fontsampler.py --font MyFont.ttf --output my_font.c
```

The generated source exposes a function named from the font and its source
size. Declare it in your application, then create an LVGL font:

```c
#include <lv_pte.h>

pte_base_font * get_MyFont(void);

lv_font_t * ui_font = lv_pte_create(get_MyFont(), 28);
lv_obj_set_style_text_font(label, ui_font, 0);
```

The generated `pte_base_font` and its arrays must remain valid until the LVGL
font is destroyed. Generated fonts are static, so this normally requires no
special handling.

Resize an existing font and notify LVGL that styles using it changed:

```c
lv_pte_set_size(ui_font, 42);
lv_obj_report_style_change(&text_style);
```

Call `lv_pte_destroy(ui_font)` after every object and style using the font has
been removed. On LVGL 9.5 and newer, `lv_pte_font_class` is also available for
applications using LVGL's font manager.

## CMake test

The adapter test uses an external LVGL checkout; LVGL is not vendored here:

```sh
cmake -S . -B build -DPTE_BUILD_LVGL_TESTS=ON -DLVGL_DIR=/path/to/lvgl
cmake --build build
ctest --test-dir build --output-on-failure
```

The adapter targets LVGL 9.4 or newer.
