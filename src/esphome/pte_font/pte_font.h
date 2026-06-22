#pragma once

#include "esphome/core/component.h"
#include "lv_pte.h"

extern "C" {
pte_base_font *get_Roboto_Regular(void);
pte_base_font *get_Roboto_Bold(void);
pte_base_font *get_Roboto_Italic(void);
pte_base_font *get_Roboto_Bold_Italic(void);
pte_base_font *get_Material_Icons(void);
}

namespace esphome::pte_font {

class PteFont : public Component {
 public:
  PteFont(const pte_base_font *source, int size) : source_(source), size_(size) {
    this->initialized_ = lv_pte_init(&this->lv_font_, source, size);
  }

  ~PteFont() {
    if (this->initialized_)
      lv_pte_deinit(&this->lv_font_);
  }

  void setup() override;
  void dump_config() override;

  void set_size(int size);
  int get_size() const { return this->size_; }
  const lv_font_t *get_lv_font() const { return &this->lv_font_; }

 protected:
  const pte_base_font *source_;
  int size_;
  bool initialized_{false};
  lv_font_t lv_font_{};
};

}  // namespace esphome::pte_font

namespace esphome::lvgl {

inline void lv_obj_set_style_text_font(lv_obj_t *obj, const pte_font::PteFont *font,
                                       lv_style_selector_t selector) {
  ::lv_obj_set_style_text_font(obj, font->get_lv_font(), selector);
}

inline void lv_style_set_text_font(lv_style_t *style, const pte_font::PteFont *font) {
  ::lv_style_set_text_font(style, font->get_lv_font());
}

}  // namespace esphome::lvgl
