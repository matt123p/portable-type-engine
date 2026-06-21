#include "pte_demo_font.h"

/* A small 5 x 7 PTE font containing E, P, and T. */
static const unsigned char pte_data[] = {
    0x00, 0x64, 0x14, 0x41, 0x14, 0x14,
    0x00, 0x41, 0x13, 0x23, 0x51, 0x14, 0x14, 0x14,
    0x00, 0x52, 0x14, 0x14, 0x14, 0x14, 0x14, 0x12,
};

static const pte_glyph pte_glyphs[] = {
    { 'E', 5, 7, 0, 7, 6, 0 },
    { 'P', 5, 7, 0, 7, 6, 6 },
    { 'T', 5, 7, 0, 7, 6, 14 },
};

static const pte_base_font pte_font = {
    7, pte_data, 3, pte_glyphs, 0, 0, 9, 7
};

const pte_base_font * get_pte_demo_font(void)
{
    return &pte_font;
}
