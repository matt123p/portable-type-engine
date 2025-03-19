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
#ifndef FONTSAMPLER_H
#define FONTSAMPLER_H

#include <QFont>
#include <map>
#include <vector>
#include <QTextCodec>

#include "Font.h"
#include "selectcharacters.h"

class FontSampler
{
private:
    std::map<int,glyph>         m_glyphs;
    std::vector<unsigned char>  m_data;
    std::vector<kern>           m_kerns;
    size_t                      m_uncompressed_size;
    QTextCodec*                 m_encoding;
    QTextEncoder*               m_encoder;
    QTextDecoder*               m_decoder;



    int val( QChar c );

    void output_pixel( bool &run_of_on, int &pixels_so_far, unsigned char &pixel);
    void convertGlyph( QFont font, QChar c );
    void calcKern( QFont font, QChar a, QChar b );

public:
    FontSampler();
    ~FontSampler();

    std::vector<intPair>        m_charSelection;
    void convertFont( QFont font, QString filename, QString encoding );

};

#endif // FONTSAMPLER_H
