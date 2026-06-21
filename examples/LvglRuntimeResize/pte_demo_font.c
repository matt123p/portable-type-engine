#include "pte_demo_font.h"

static const unsigned char pte_data[] = {
    0x06, 0x41, 0x44, 0x11, 0x41, 0x44, 0x10,
    0x04, 0x11, 0x32, 0x35, 0x11, 0x41, 0x41, 0x40,
    0x05, 0x21, 0x41, 0x41, 0x41, 0x41, 0x41, 0x20,
};
static const pte_glyph pte_glyphs[] = {
    { 'E', 5, 7, 0, 7, 6, 0 },
    { 'P', 5, 7, 0, 7, 6, 7 },
    { 'T', 5, 7, 0, 7, 6, 15 },
};
static const pte_base_font pte_font = {
    7, pte_data, 3, pte_glyphs, 0, 0, 9, 7
};
const pte_base_font * get_pte_demo_font(void) { return &pte_font; }
