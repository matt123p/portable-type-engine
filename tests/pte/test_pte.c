#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <pte/pte.h>

static const pte_glyph glyphs[] = {
	{'A', 0, 0, 0, 0, 3, 0},
	{'V', 0, 0, 0, 0, 3, 0},
};

static const uint8_t compact_glyph_rows[] = {0, PTE_NO_COMPACT_KERN_ROW};
static const uint16_t compact_row_offsets[] = {0, 2};
static const pte_compact_kern_entry compact_entries[] = {
	PTE_COMPACT_KERN_ENTRY(0, 3),
	PTE_COMPACT_KERN_ENTRY(1, -1),
};
static const pte_base_font compact_source = {
	1, NULL, 2, glyphs,
	compact_glyph_rows, compact_row_offsets, compact_entries,
	2, 1, PTE_KERN_FORMAT_COMPACT
};

static const uint16_t legacy_glyph_rows[] = {0, PTE_NO_KERN_ROW};
static const pte_kern_row legacy_rows[] = {{0, 2}};
static const pte_kern_entry legacy_entries[] = {{0, 3}, {1, -1}};
/* Deliberately omit the trailing format field, as existing generated fonts do. */
static const pte_base_font legacy_source = {
	1, NULL, 2, glyphs,
	legacy_glyph_rows, legacy_rows, legacy_entries,
	2, 1
};

#define LARGE_GLYPH_COUNT 257
static pte_glyph large_glyphs[LARGE_GLYPH_COUNT];
static uint8_t large_glyph_rows[LARGE_GLYPH_COUNT];
static const uint16_t large_row_offsets[] = {0, 1};
static const pte_compact_kern_entry large_entries[] = {
	PTE_COMPACT_KERN_ENTRY(64, -1),
};
static const pte_base_font large_source = {
	1, NULL, LARGE_GLYPH_COUNT, large_glyphs,
	large_glyph_rows, large_row_offsets, large_entries,
	2, 1, PTE_KERN_FORMAT_COMPACT
};

static const unsigned char bitmap_data[] = {0x40, 0x84, 0x65, 0x10};
static const pte_glyph bitmap_glyphs[] = {
	{'X', 2, 3, 0, 0, 2, 0},
};
static const pte_base_font bitmap_source = {
	1, bitmap_data, 1, bitmap_glyphs,
	NULL, NULL, NULL,
	3, 3, PTE_KERN_FORMAT_LEGACY
};

static int blended_pixels;
static int blended_x[3];
static int blended_y[3];

void hw_blendPixel(int x, int y, int alpha, int colour)
{
	(void)colour;
	assert(alpha > 0);
	assert(blended_pixels < 3);
	blended_x[blended_pixels] = x;
	blended_y[blended_pixels] = y;
	++blended_pixels;
}

static void assert_measurement(const pte_base_font* source)
{
	pte_font font = pte_getFont(source, 1);
	int width;
	int height;

	pte_measureText(&font, "AV", (size_t)-1, &width, &height);
	assert(width == 5);
	assert(height == 2);

	pte_measureText(&font, "AA", (size_t)-1, &width, &height);
	assert(width == 9);
	assert(height == 2);
}

int main(void)
{
	int i;
	pte_font font;
	int width;
	int height;

	assert_measurement(&compact_source);
	assert_measurement(&legacy_source);

	for (i = 0; i < LARGE_GLYPH_COUNT; ++i)
	{
		large_glyphs[i].code = i + 1;
		large_glyphs[i].xadvance = 3;
		large_glyph_rows[i] = PTE_NO_COMPACT_KERN_ROW;
	}
	large_glyph_rows[256] = 0;
	font = pte_getFont(&large_source, 1);
	pte_measureText(&font, "\xc4\x81" "A", (size_t)-1, &width, &height);
	assert(width == 5);
	assert(height == 2);

	font = pte_getFont(&bitmap_source, 1);
	blended_pixels = 0;
	pte_drawText(&font, 0, 3, 0, "X", (size_t)-1, 0);
	assert(blended_pixels == 3);
	assert(blended_x[0] == 0 && blended_y[0] == 4);
	assert(blended_x[1] == 0 && blended_y[1] == 5);
	assert(blended_x[2] == 1 && blended_y[2] == 6);

	return 0;
}
