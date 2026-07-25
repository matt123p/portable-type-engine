#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <lv_pte.h>

static const unsigned char data[] = {
    0x40, 0x84, 0x80,
    0x40, 0x84, 0x80,
};
static const pte_glyph glyphs[] = {
    {'A', 2, 2, 0, 0, 3, 0},
    {'V', 2, 2, 0, 0, 3, 3},
};
static const uint8_t glyph_kern_rows[] = {0, PTE_NO_COMPACT_KERN_ROW};
static const uint16_t kern_rows[] = {0, 2};
static const pte_compact_kern_entry kern_entries[] = {
    PTE_COMPACT_KERN_ENTRY(0, 3),
    PTE_COMPACT_KERN_ENTRY(1, -1),
};
static const pte_base_font source = {
    1, data, 2, glyphs, glyph_kern_rows, kern_rows, kern_entries, 2, 1,
    PTE_KERN_FORMAT_COMPACT
};
static const uint16_t legacy_glyph_kern_rows[] = {0, PTE_NO_KERN_ROW};
static const pte_kern_row legacy_kern_rows[] = {{0, 2}};
static const pte_kern_entry legacy_kern_entries[] = {{0, 3}, {1, -1}};
static const pte_base_font legacy_source = {
    1, data, 2, glyphs,
    legacy_glyph_kern_rows, legacy_kern_rows, legacy_kern_entries,
    2, 1
};

int main(void)
{
    lv_init();
    lv_font_t * font = lv_pte_create(&source, 2);
    assert(font != NULL);
    assert(font->line_height == 4);
    assert(font->base_line == 2);

    lv_font_glyph_dsc_t dsc;
    assert(lv_font_get_glyph_dsc(font, &dsc, 'A', 'V'));
    assert(dsc.adv_w == 4);
    assert(lv_font_get_glyph_dsc(font, &dsc, 'A', 'A'));
    assert(dsc.adv_w == 12);
    assert(dsc.format == LV_FONT_GLYPH_FORMAT_A8);
    assert(dsc.box_w > 0 && dsc.box_h > 0);

    uint8_t bitmap[64];
    memset(bitmap, 0, sizeof(bitmap));
    lv_draw_buf_t draw_buf = {
        .header.stride = 8,
        .data_size = sizeof(bitmap),
        .data = bitmap,
    };
    assert(lv_font_get_glyph_bitmap(&dsc, &draw_buf) != NULL);
    assert(bitmap[0] + bitmap[1] + bitmap[8] + bitmap[9] > 0);

    lv_font_set_kerning(font, LV_FONT_KERNING_NONE);
    assert(lv_font_get_glyph_dsc(font, &dsc, 'A', 'V'));
    assert(dsc.adv_w == 6);
    assert(!lv_font_get_glyph_dsc(font, &dsc, 'B', 0));

    lv_pte_set_size(font, 3);
    assert(font->line_height == 6);
    assert(font->base_line == 3);
    lv_pte_destroy(font);

    font = lv_pte_create(&legacy_source, 2);
    assert(font != NULL);
    assert(lv_font_get_glyph_dsc(font, &dsc, 'A', 'V'));
    assert(dsc.adv_w == 4);
    assert(lv_font_get_glyph_dsc(font, &dsc, 'A', 'A'));
    assert(dsc.adv_w == 12);
    lv_pte_destroy(font);

    lv_font_t in_place;
    memset(&in_place, 0xa5, sizeof(in_place));
    assert(lv_pte_init(&in_place, &source, 4));
    assert(in_place.line_height == 8);
    assert(in_place.base_line == 4);
    assert(lv_font_get_glyph_dsc(&in_place, &dsc, 'A', 0));
    lv_pte_deinit(&in_place);
    assert(in_place.dsc == NULL);
    assert(in_place.get_glyph_dsc == NULL);

    lv_deinit();
    return 0;
}
