/*
Copyright (c) 2015, Matt Pyne
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse
or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include "pte.h"

// Find a character in a font
static const pte_glyph* findChar(int c, const pte_base_font* f)
{
	// Do a binary search to find the character
	int l = 0;
	int h = f->m_number_glyphs - 1;

	// Is it a top or bottom code?
	if (c == f->m_gylphs[l].code)
	{
		return &(f->m_gylphs[l]);
	}
	if (c == f->m_gylphs[h].code)
	{
		return &(f->m_gylphs[h]);
	}

	// Is it outside of this array?
	if (c < f->m_gylphs[l].code)
	{
		// We don't have the character
		return NULL;
	}
	if (c > f->m_gylphs[h].code)
	{
		// We don't have the character
		return NULL;
	}

	while (h - l > 1)
	{
		// Find a the mid-point
		int m = (h - l) / 2 + l;

		// Do we have it?
		if (c == f->m_gylphs[m].code)
		{
			return &(f->m_gylphs[m]);
		}

		// Ok, higher or lower?
		if (c < f->m_gylphs[m].code)
		{
			// Lower
			h = m;
		}
		else
		{
			// Higher
			l = m;
		}
	}

	// Not found!
	return NULL;
}

static int compactKernAmount(pte_compact_kern_entry entry)
{
	unsigned int amount = (unsigned int)(entry >> 8);
	return amount < 0x80U ? (int)amount : (int)amount - 0x100;
}

static int findKern(int first_glyph, int second_glyph, const pte_base_font* f)
{
	uint32_t low;
	uint32_t high;

	if (first_glyph < 0 || second_glyph < 0
		|| first_glyph >= f->m_number_glyphs || second_glyph >= f->m_number_glyphs
		|| !f->m_glyph_kern_rows || !f->m_kern_rows || !f->m_kern_entries)
	{
		return 0;
	}

	if (f->m_kern_format == PTE_KERN_FORMAT_COMPACT)
	{
		const uint8_t* glyph_rows = (const uint8_t*)f->m_glyph_kern_rows;
		const uint16_t* row_offsets = (const uint16_t*)f->m_kern_rows;
		const pte_compact_kern_entry* entries =
			(const pte_compact_kern_entry*)f->m_kern_entries;
		uint8_t row_index = glyph_rows[first_glyph];

		if (row_index == PTE_NO_COMPACT_KERN_ROW || second_glyph > UINT8_MAX)
		{
			return 0;
		}

		low = row_offsets[row_index];
		high = row_offsets[(uint16_t)row_index + 1U];
		while (low < high)
		{
			uint32_t mid = low + (high - low) / 2;
			pte_compact_kern_entry entry = entries[mid];
			uint8_t entry_second = (uint8_t)entry;
			if (entry_second == (uint8_t)second_glyph)
			{
				return compactKernAmount(entry);
			}
			if (entry_second < (uint8_t)second_glyph)
			{
				low = mid + 1;
			}
			else
			{
				high = mid;
			}
		}
		return 0;
	}

	{
		const uint16_t* glyph_rows = (const uint16_t*)f->m_glyph_kern_rows;
		const pte_kern_row* rows = (const pte_kern_row*)f->m_kern_rows;
		const pte_kern_entry* entries = (const pte_kern_entry*)f->m_kern_entries;
		const pte_kern_row* row;
		uint16_t row_index = glyph_rows[first_glyph];

		if (row_index == PTE_NO_KERN_ROW)
		{
			return 0;
		}

		row = &rows[row_index];
		low = row->offset;
		high = row->offset + row->count;
		while (low < high)
		{
			uint32_t mid = low + (high - low) / 2;
			const pte_kern_entry* entry = &entries[mid];
			if (entry->second_glyph == (uint16_t)second_glyph)
			{
				return entry->amount;
			}
			if (entry->second_glyph < (uint16_t)second_glyph)
			{
				low = mid + 1;
			}
			else
			{
				high = mid;
			}
		}
	}

	return 0;
}

#define PTE_RESAMPLE_ALLOC malloc
#define PTE_RESAMPLE_FREE free
#include "pte_resample.h"

// Bitblt one Y sampling group, retaining the original X/Y padding and alpha.
static void blt_horz_cmprs_resize(pte_bitmap_decoder* decoder, pte_resample_buffer* buffer,
	int src_width, int dst_x, int dst_y, int pixel_xinc, int pixel_yinc,
	int ra, int rb, int sub_offset_x, int lines, int overspill, int plot_col)
{
	if (!decoder->buckets && !pteResamplePrepare(decoder, buffer, src_width, ra, rb, sub_offset_x))
	{
		return;
	}
	pteResampleRows(decoder, lines);
	for (int p = 0; p < decoder->count; ++p)
	{
		const pte_resample_bucket* bucket = &decoder->buckets[p];
		int div = bucket->span * (lines + overspill);
		if (bucket->total > 0)
		{
			hw_blendPixel(dst_x, dst_y, (bucket->total << 8) / div, plot_col);
		}
		dst_x += pixel_xinc;
		dst_y += pixel_yinc;
	}
}

// Decode one UTF-8 code point. Invalid sequences fall back to their first byte.
static int nextChar(const char* text, size_t available, int* bytes)
{
	const unsigned char* s = (const unsigned char*)text;
	int length;
	int code;
	int i;

	*bytes = 1;
	if (s[0] < 0x80)
	{
		return s[0];
	}
	if (s[0] >= 0xc2 && s[0] <= 0xdf)
	{
		length = 2;
		code = s[0] & 0x1f;
	}
	else if (s[0] >= 0xe0 && s[0] <= 0xef)
	{
		length = 3;
		code = s[0] & 0x0f;
	}
	else if (s[0] >= 0xf0 && s[0] <= 0xf4)
	{
		length = 4;
		code = s[0] & 0x07;
	}
	else
	{
		return s[0];
	}

	if (available < (size_t)length)
	{
		return s[0];
	}
	for (i = 1; i < length; ++i)
	{
		if (s[i] == 0 || (s[i] & 0xc0) != 0x80)
		{
			return s[0];
		}
		code = (code << 6) | (s[i] & 0x3f);
	}
	if ((length == 3 && code < 0x800)
		|| (length == 4 && code < 0x10000)
		|| (code >= 0xd800 && code <= 0xdfff)
		|| code > 0x10ffff)
	{
		return s[0];
	}

	*bytes = length;
	return code;
}

// Draw text on the canvas
int pte_drawText(pte_font* f, int x, int y, int r, const char* text, size_t size, int c)
{
	pte_resample_buffer buffer = {NULL, 0};
	int last_glyph = -1;
	size_t i;
	const pte_base_font* bf = f->m_font;

	int pixel_xinc = 1;
	int pixel_yinc = 0;
	int line_xinc = 0;
	int line_yinc = 1;
	switch (r)
	{
	case 90:
		pixel_xinc = 0;
		pixel_yinc = 1;
		line_xinc = -1;
		line_yinc = 0;
		break;
	case 180:
		pixel_xinc = -1;
		pixel_yinc = 0;
		line_xinc = 0;
		line_yinc = -1;
		break;
	case 270:
		pixel_xinc = 0;
		pixel_yinc = -1;
		line_xinc = 1;
		line_yinc = 0;
		break;
	default:
		break;
	}

	x = (x * f->m_rb) / f->m_ra;
	y = (y * f->m_rb) / f->m_ra;
	for (i = 0; (size == (size_t)-1 || i < size) && text[i] != 0;)
	{
		int bytes;
		int character = nextChar(text + i,
			size == (size_t)-1 ? (size_t)-1 : size - i, &bytes);
		const pte_glyph* g = findChar(character, bf);
		i += bytes;
		if (g)
		{
			// Bitblt this character across
			int kern;
			int glyph_index = (int)(g - bf->m_gylphs);

			int acc = 0;
			pte_bitmap_decoder decoder;
			int cy = 0;
			int last_cy = 0;
			int finished = 0;
			int offset_x;
			int sub_offset_x;
			int offset_y;
			int sub_offset_y;
			int xoffset = g->xoffset;
			int yoffset = g->yoffset;
			int sub_offset_dx;
			int sub_offset_dy;

			bitmapDecoderInit(&decoder, bf, g);
			kern = findKern(last_glyph, glyph_index, bf);
			x += kern * pixel_xinc;
			y += kern * pixel_yinc;

			switch (r)
			{
			case 90:
				offset_y = ((y + g->xoffset) * f->m_ra) / f->m_rb;
				sub_offset_y = ((y + g->xoffset) * f->m_ra) % f->m_rb;
				offset_x = ((x + g->yoffset) * f->m_ra) / f->m_rb;
				sub_offset_x = ((x + g->yoffset) * f->m_ra) % f->m_rb;
				if (!sub_offset_x)
				{
					--offset_x;
				}
				break;
			case 180:
				offset_x = ((x - g->xoffset) * f->m_ra) / f->m_rb;
				sub_offset_x = f->m_rb - ((x - g->xoffset) * f->m_ra) % f->m_rb - 1;
				offset_y = ((y + g->yoffset) * f->m_ra) / f->m_rb;
				sub_offset_y = ((y + g->yoffset) * f->m_ra) % f->m_rb;
				if (sub_offset_x)
				{
					--offset_x;
				}
				if (sub_offset_y)
				{
					++offset_y;
				}
				break;
			case 270:
				offset_x = ((x - g->yoffset) * f->m_ra) / f->m_rb;
				sub_offset_x = f->m_rb - ((x - g->yoffset) * f->m_ra) % f->m_rb - 1;
				offset_y = ((y - g->xoffset) * f->m_ra) / f->m_rb;
				sub_offset_y = f->m_rb - ((y - g->xoffset) * f->m_ra) % f->m_rb - 1;
				if (sub_offset_y)
				{
					--offset_y;
				}
				break;
			default:
				offset_x = ((x + g->xoffset) * f->m_ra) / f->m_rb;
				sub_offset_x = ((x + g->xoffset) * f->m_ra) % f->m_rb;
				offset_y = ((y - g->yoffset) * f->m_ra) / f->m_rb;
				sub_offset_y = f->m_rb - ((y - g->yoffset) * f->m_ra) % f->m_rb - 1;
				if (sub_offset_y)
				{
					--offset_y;
				}
				break;
			}

			switch (r)
			{
			case 270:
			case 90:
				sub_offset_dx = sub_offset_y;
				sub_offset_dy = sub_offset_x;
				break;
			default:
				sub_offset_dx = sub_offset_x;
				sub_offset_dy = sub_offset_y;
				break;
			}

			acc = f->m_rb;
			while (!finished)
			{
				acc -= f->m_ra;

				if (acc <= 0)
				{
					int lines = cy - last_cy;
					int overspill = 0;

					if (sub_offset_dy > 0)
					{
						overspill = cy - (sub_offset_dy * cy) / f->m_rb;
						lines -= overspill;
						cy -= overspill;
						sub_offset_dy = 0;
					}
					else if (cy >= g->height)
					{
						overspill = cy - g->height;
						lines -= overspill;
						finished = 1;
					}

					if (lines > 0)
					{
						blt_horz_cmprs_resize(&decoder, &buffer, g->width,
							offset_x, offset_y, pixel_xinc, pixel_yinc,
							f->m_ra, f->m_rb, sub_offset_dx, lines, overspill, c);
					}
					offset_x += line_xinc;
					offset_y += line_yinc;

					last_cy = cy;
					acc += f->m_rb;
				}

				++cy;
			}

			switch (r)
			{
			case 90:
				y += g->xadvance;
				break;
			case 180:
				x -= g->xadvance;
				break;
			case 270:
				y -= g->xadvance;
				break;
			default:
				x += g->xadvance;
				break;
			}

			last_glyph = glyph_index;
		}
	}

	free(buffer.buckets);
	return (x * f->m_ra) / f->m_rb;
}

// How big will the text be?
void pte_measureText(pte_font* f, const char* text, size_t size, int* dx, int* dy)
{
	int last_glyph = -1;
	size_t i;
	const pte_base_font* bf = f->m_font;
	*dx = 0;
	for (i = 0; (size == (size_t)-1 || i < size) && text[i] != 0;)
	{
		int bytes;
		int character = nextChar(text + i,
			size == (size_t)-1 ? (size_t)-1 : size - i, &bytes);
		const pte_glyph* g = findChar(character, bf);
		i += bytes;
		if (g)
		{
			int glyph_index = (int)(g - bf->m_gylphs);
			int kern = findKern(last_glyph, glyph_index, bf);
			*dx += g->xadvance + kern;

			last_glyph = glyph_index;
		}
	}

	*dy = f->m_line_height;
	*dx = (*dx * f->m_ra) / f->m_rb;
}

// Centre the text in a rectangle
void pte_drawTextRect(pte_Placement o, pte_font* f, int x1, int y1, int x2, int y2, int r, const char* text, size_t size, int c)
{
	int dx, dy;
	int x = 0, y = 0;
	pte_measureText(f, text, size, &dx, &dy);

	switch (r)
	{
	case 270:
	case 90:
		// Swap x1, y1 and x2, y2
	{
		int t = x1;
		x1 = y1;
		y1 = t;
		t = x2;
		x2 = y2;
		y2 = t;
	}
	break;
	}

	switch (o & 0xf)
	{
	case TEXT_VCENTER:
		x = ((x2 - x1) - dx) / 2 + x1;
		break;
	case TEXT_LEFT:
		x = x1;
		break;
	case TEXT_RIGHT:
		x = x2 - dx - 1;
		break;
	}

	switch (o & 0xf0)
	{
	case TEXT_HCENTER:
		y = ((y2 - y1) - f->m_baseline) / 2 + y1;
		break;
	case TEXT_TOP:
		y = y1;
		break;
	case TEXT_BOTTOM:
		y = y2 - f->m_baseline;
		break;
	}
	y += f->m_baseline;

	pte_drawText(f, x, y, r, text, size, c);
}

void pte_drawTextRectWrapped(pte_Placement o, pte_font* f, int x1, int y1, int x2, int y2, int r, const char* text, size_t size, int c)
{
	int rect_width = x2 - x1;
	int line_height = f->m_line_height;
	const char* word_start = text;
	const char* line_start = text;
	const char* word_end = text;
	int dx, dy;

	while (*word_start && y1 < (y2 - line_height))
	{
		word_end = word_start;
		while (*word_end && *word_end != ' ' && *word_end != '\n')
		{
			word_end++;
		}
		size_t prev_length = word_start - line_start;
		size_t line_length = word_end - line_start;

		pte_measureText(f, line_start, line_length, &dx, &dy);
		if (dx > rect_width)
		{
			if (line_start == word_start)
			{
				// Single word is too long to fit in the line, force break
				word_start = word_end;
			}
			else
			{
				// Draw the current line and start a new one
				pte_drawTextRect(o, f, x1, y1, x2, y2, r, line_start, prev_length, c);
				y1 += line_height;
				line_start = word_start;
				continue;
			}
		}
		else
		{
			word_start = word_end;
			if (*word_start == ' ')
			{
				word_start++;
			}
		}

		if (*word_start == '\n')
		{
			pte_drawTextRect(o, f, x1, y1, x2, y2, r, line_start, line_length, c);
			y1 += line_height;
			word_start++;
			line_start = word_start;
		}
	}

	if (y1 < (y2 - line_height) && line_start < word_start)
	{
		size_t line_length = word_end - line_start;
		pte_drawTextRect(o, f, x1, y1, x2, y2, r, line_start, line_length, c);
	}
}

// Get a font
pte_font pte_getFont(const pte_base_font* f, int size)
{
	pte_font r;
	r.m_ra = size;
	r.m_rb = f->m_size;
	r.m_font = f;
	r.m_line_height = (f->m_line_height * r.m_ra) / r.m_rb;
	r.m_baseline = (f->m_baseline * r.m_ra) / r.m_rb;

	return r;
}
