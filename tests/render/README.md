# Golden-image rendering tests

These tests render realistic text scenes with all four bundled Roboto styles and
compare every generated pixel with the committed PNG images in `golden/`.

The scenes cover body text at sizes 12, 18, 24 and 32, dashboard labels, large
changing numbers, fractional sizes, accented UTF-8 text, punctuation, kerning,
and all four rotations supported by the native renderer. The LVGL suite also
renders through the public glyph metrics and A8 bitmap callbacks.

Raw LVGL glyph output is an opacity mask, so the test converts it to dark text on
white when producing the comparison image. The conversion is only presentation;
the renderer's coverage values are unchanged.

Build and run the native tests:

```sh
cmake -S . -B build -DPTE_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Add LVGL coverage with:

```sh
cmake -S . -B build -DPTE_BUILD_TESTS=ON -DPTE_BUILD_LVGL_TESTS=ON \
  -DLVGL_DIR=/path/to/lvgl
```

`PTE_BUILD_RENDER_TESTS` follows `PTE_BUILD_TESTS` by default and can be disabled
explicitly for environments without Python 3. The comparator uses only Python's
standard library and writes generated files to a temporary directory.
