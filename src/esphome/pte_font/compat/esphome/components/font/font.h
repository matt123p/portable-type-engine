#pragma once

#include <lvgl.h>

namespace esphome::font {

class Font {
 public:
  const lv_font_t *get_lv_font() const;
};

}  // namespace esphome::font
