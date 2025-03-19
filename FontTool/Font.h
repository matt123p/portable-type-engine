/*
    FontTool for the Portable Font Engine.
    Copyright (C) 2015  Matt Pyne

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _FONT_H_
#define _FONT_H_

typedef struct 
{
    QChar c;
    int code;
	int width;
	int height;
	int xoffset;
	int yoffset;
	int xadvance;
    int ptr;
} glyph;

typedef struct
{
	int first;
	int second;

	int amount;
} kern;

typedef struct 
{
	// The size of the font
	int						m_size;

	// The actual font data
    const unsigned char*	m_data;

	// The glyph data
	int				m_number_glyphs;
	const glyph*	m_gylphs;

	// The kerning data
	int				m_number_kerns;
	const kern*		m_kerns;

	// Placement
	int				m_line_height;
	int				m_baseline;

} base_font;

typedef struct _font
{
	// The base font
    const base_font*	m_font;

	// Resizing data
	int				m_ra;
	int				m_rb;

	// Placement (of resized font)
	int				m_line_height;
	int				m_baseline;
} font;

#define SMALLEST_FONT	16
#define SMALL_FONT		20
#define SMEDIUM_FONT	28
#define	MEDIUM_FONT		30
#define MBIG_FONT		34
#define BIG_FONT		50
#define	MLARGE_FONT		80
#define	LARGE_FONT		100

#define MEDIUM_ICONS	30
#define LARGE_ICONS		100

base_font *get_courier100();
base_font *get_oxygen128();

font get_oxygen( int size );
font get_courier( int size );

#endif
