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
static const pte_glyph* findChar(unsigned char c, const pte_base_font* f)
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

static int searchKern(unsigned char c, const pte_base_font* f)
{
	// Do a binary search to find the character
	int l = 0;
	int h = f->m_number_kerns - 1;

	if (!f->m_kerns || f->m_number_kerns == 0)
	{
		return -1;
	}

	// Is it a top or bottom code?
	if (c == f->m_kerns[l].first)
	{
		return l;
	}
	if (c == f->m_kerns[h].first)
	{
		return h;
	}

	// Is it outside of this array?
	if (c < f->m_kerns[l].first)
	{
		// We don't have the character
		return -1;
	}
	if (c > f->m_kerns[h].first)
	{
		// We don't have the character
		return -1;
	}

	while (h - l > 1)
	{
		// Find a the mid-point
		int m = (h - l) / 2 + l;

		// Do we have it?
		if (c == f->m_kerns[m].first)
		{
			return m;
		}

		// Ok, higher or lower?
		if (c < f->m_kerns[m].first)
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
	return -1;
}

static const pte_kern* findKern(int c1, int c2, const pte_base_font* f)
{
	// Find the first kern of this type in the array
	int mid_p = searchKern(c1, f);

	// Now scan up/down to see if we have the pair
	int d;
	for (d = -1; d <= 1; d += 2)
	{
		int p = mid_p;
		while (p != -1
			&& p != f->m_number_kerns
			&& f->m_kerns[p].first == c1)
		{
			if (f->m_kerns[p].second == c2)
			{
				return &(f->m_kerns[p]);
			}
			p += d;
		}
	}

	return NULL;
}

struct decomp_line
{
	const unsigned char* raw_p;
	int pixels_to_go;
	int col;
	int acc;
};

// Bitblt a horizontal line from a compressed source
static void blt_horz_cmprs_resize(const unsigned char** ptr, int* col, int* pixels_to_go, int src_width, int dst_x, int dst_y, int ra, int rb, int sub_offset_x, int lines, int overspill, int plot_col)
{
	unsigned char line_acc[128];

	int x_start = -((rb * sub_offset_x) / ra) / rb;
	int x = x_start;
	int c = rb;
	int p = 0;
	int count = lines;
	int div = 0;

	memset(line_acc, 0, sizeof(line_acc));

	// Accumulate a horizontal line of data
	while (count > 0)
	{
		if (x >= 0 && x < src_width)
		{
			// Decode the compressed font data
			while (*pixels_to_go == 0)
			{
				if (!*col)
				{
					++ (*ptr);
				}
				*col = !*col;
				if (*col)
				{
					*pixels_to_go = (**ptr) >> 4;
				}
				else
				{
					*pixels_to_go = (**ptr) & 0xf;
				}
			}

			if (*col)
			{
				++line_acc[p];
			}

			-- (*pixels_to_go);
		}
		div += (lines + overspill);

		++x;

		// Move on the divider
		c -= ra;

		// Do we output a pixel, either because of the counter
		// or because this is the last pixel (y == 0)?
		if (c <= 0 || count == 0)
		{
			// Are we on the the last line we are looking at?
			if (count <= 1)
			{
				// Yes, so output a pixel
				if (line_acc[p] > 0)
				{
					hw_blendPixel(dst_x, dst_y, (line_acc[p] << 8) / div, plot_col);
				}

				++dst_x;
			}

			// Reset the counters
			div = 0;
			c += rb;
			++p;

			if (x >= src_width)
			{
				// Move to the next line
				--count;

				// Reset the counters (unless this is the last pixel)
				p = 0;
				div = 0;
				x = x_start;
				c = rb;
			}
		}
	}
}

// Draw text on the canvas
int pte_drawText(pte_font* f, int x, int y, const char* text, int size, int c)
{
	int last_char = -1;
	int i;
	const pte_base_font* bf = f->m_font;

	x = (x * f->m_rb) / f->m_ra;
	for (i = 0; text[i] != 0 && (size == -1 || i < size); ++i)
	{
		const pte_glyph* g = findChar(text[i], bf);
		if (g)
		{
			// Bitblt this character across
			const pte_kern* k;

			int acc = 0;
			int col = 1;
			int pixels_to_go = 0;
			int offset_y = -(g->yoffset * f->m_ra) / f->m_rb;
			int sub_offset_y = (g->yoffset * f->m_ra) % f->m_rb;
			int cy = 0;
			int last_cy = 0;
			int finished = 0;
			const unsigned char* ptr = bf->m_data + g->ptr;
			int sub_offset_x;
			int offset_x;

			k = findKern(last_char, text[i], bf);
			if (k)
			{
				x += k->amount;
			}

			offset_x = ((x + g->xoffset) * f->m_ra) / f->m_rb;
			sub_offset_x = ((x + g->xoffset) * f->m_ra) % f->m_rb;

			if (sub_offset_y)
			{
				--offset_y;
			}
			acc = f->m_rb;
			while (!finished)
			{
				acc -= f->m_ra;

				if (acc <= 0)
				{
					int lines = cy - last_cy;
					int overspill = 0;

					if (sub_offset_y > 0)
					{
						overspill = cy - (sub_offset_y * cy) / f->m_rb;
						lines -= overspill;
						cy -= overspill;
						sub_offset_y = 0;
					}
					else if (cy >= g->height)
					{
						overspill = cy - g->height;
						lines -= overspill;
						finished = 1;
					}

					if (lines > 0)
					{
						blt_horz_cmprs_resize(&ptr, &col, &pixels_to_go, g->width, offset_x, y + offset_y, f->m_ra, f->m_rb, sub_offset_x, lines, overspill, c);
					}
					++offset_y;

					last_cy = cy;
					acc += f->m_rb;
				}

				++cy;
			}

			x += g->xadvance;

			last_char = text[i];
		}
	}

	return (x * f->m_ra) / f->m_rb;
}

// How big will the text be?
void pte_measureText(pte_font* f, const char* text, int size, int* dx, int* dy)
{
	int last_char = -1;
	int i;
	const pte_base_font* bf = f->m_font;
	*dx = 0;
	for (i = 0; text[i] != 0 && (size == -1 || i < size); ++i)
	{
		const pte_glyph* g = findChar(text[i], bf);
		if (g)
		{
			const pte_kern* k;
			k = findKern(last_char, text[i], bf);
			if (k)
			{
				*dx += g->xadvance + k->amount;
			}
			else
			{
				*dx += g->xadvance;
			}

			last_char = text[i];
		}
	}

	*dy = f->m_line_height;
	*dx = (*dx * f->m_ra) / f->m_rb;
}

// Centre the text in a rectangle
int pte_drawTextRect(pte_Placement o, pte_font* f, int x1, int y1, int x2, int y2, const char* text, int size, int c)
{
	int dx, dy;
	int x = 0, y = 0;
	pte_measureText(f, text, size, &dx, &dy);

	switch (o & 0xf)
	{
	case TEXT_VCENTRE:
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
	case TEXT_HCENTRE:
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

	return pte_drawText(f, x, y, text, size, c);
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