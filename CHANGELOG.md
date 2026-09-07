# Changelog

## [2.2.0] - 2026-09-07

### Changed

- Significantly improved glyph rendering performance by consuming compressed
  bitmap runs in batches instead of decoding one source pixel at a time.
- Reused horizontally resampled coverage for repeated source rows while
  preserving both X and Y resampling and the existing output.
- Reused scratch storage across destination rows, removing allocation and
  clearing work from the inner rendering loop.
- Applied the same resampling implementation to the native PTE and LVGL/ESPHome
  renderers.

### Added

- Added pixel-exact golden-image regression tests covering real-world text,
  multiple font styles and sizes, UTF-8 text, dashboard and numeric content,
  native rotations, and the public LVGL glyph callbacks.

## [2.1.1] - 2026-09-04

### Fixed

- Fixed ESPHome native ESP-IDF builds failing to find `pte.h` and `lv_pte.h` by
  copying the required engine headers into the generated source directory.
- Stopped forcing `USE_FONT`, allowing builds without an ESPHome bitmap font to
  avoid the `esphome/components/font/font.h` dependency.
- Made the LVGL adapter include `pte.h` through a portable include path.

### Upgrade note

- When upgrading an existing ESPHome build, remove its complete
  `.esphome/build/<node-name>` directory before compiling. This prevents a stale
  native `pte.c` from being built alongside the LVGL adapter.

## [2.1.0] - 2026-08-04

### Changed

- Improved bitmap compression with run-length and Golomb-Rice coding while
  preserving rendering quality.
- Compacted kerning tables and regenerated all bundled Roboto fonts, reducing
  generated font data size.
- Updated the core PTE engine and LVGL adapter and expanded their test coverage.
- Refreshed the project, example, and font-tool documentation.

### Added

- Added an ESPHome and LVGL integration guide with a complete 480p dashboard
  example using bundled text fonts and a user-supplied icon font.

### Removed

- Removed the large bundled Material Design Icons font; applications now supply
  icon fonts as demonstrated by the ESPHome example.

## [2.0.1] - 2026-06-26

- Maintenance release. The GitHub release did not include detailed change notes.

[2.2.0]: https://github.com/matt123p/portable-type-engine/compare/v2.1.1...v2.2.0
[2.1.1]: https://github.com/matt123p/portable-type-engine/releases/tag/v2.1.1
[2.1.0]: https://github.com/matt123p/portable-type-engine/releases/tag/v2.1.0
[2.0.1]: https://github.com/matt123p/portable-type-engine/releases/tag/v2.0.1
