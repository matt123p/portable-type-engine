#include "pte_font.h"

#include "esphome/core/log.h"

namespace esphome::pte_font {

static const char *const TAG = "pte_font";

void PteFont::setup() {
  if (!this->initialized_) {
    ESP_LOGE(TAG, "Failed to initialize PTE font at size %d", this->size_);
    this->mark_failed();
  }
}

void PteFont::dump_config() {
  ESP_LOGCONFIG(TAG, "Portable Type Engine font:");
  ESP_LOGCONFIG(TAG, "  Size: %d", this->size_);
}

void PteFont::set_size(int size) {
  if (!this->initialized_ || size <= 0)
    return;
  lv_pte_set_size(&this->lv_font_, size);
  this->size_ = size;
}

}  // namespace esphome::pte_font
