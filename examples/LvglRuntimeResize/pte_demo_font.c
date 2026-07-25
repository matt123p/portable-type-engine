#include "pte_demo_font.h"

static const unsigned char pte_data[15] = {
 6,133,165,153,88,66,133,165,153,88,100,133,105,153,0
};
static const pte_glyph pte_glyphs[] = {
    { 'E', 5, 7, 0, 7, 6, 0 },
    { 'P', 5, 7, 0, 7, 6, 5 },
    { 'T', 5, 7, 0, 7, 6, 10 },
};
static const pte_base_font pte_font = {
    7, pte_data, 3, pte_glyphs, 0, 0, 0, 9, 7, PTE_KERN_FORMAT_LEGACY
};
const pte_base_font * get_pte_demo_font(void) { return &pte_font; }
