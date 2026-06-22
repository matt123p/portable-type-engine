#include "pte_font.h"

namespace esphome::pte_font {

void PteFont::set_size(int size) {
  if (!this->initialized_ || size <= 0)
    return;
  lv_pte_set_size(&this->lv_font_, size);
  this->size_ = size;
}

}  // namespace esphome::pte_font
