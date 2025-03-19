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

#include "maindlg.h"
#include <qimage.h>
#include <QColor>
#include "ui_maindlg.h"

extern "C"
{
    #include "../include/pte.h"
    pte_base_font* get_Roboto128();
}


QImage     g_image;
QIcon      g_button_icon;


MainDlg::MainDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDlg)
{
    ui->setupUi(this);


    // Setup the output image
    g_image = QImage( ui->displayPanel->size(),QImage::Format_ARGB32 );
    g_image.fill(  qRgb(255,255,255) );


    //
    // Do some drawing of text
    //
    int y = 0;

    {
        pte_font f = pte_getFont( get_Roboto128(), 40 );
        y = f.m_baseline;
        pte_drawText( &f, 5,y, "Example text", -1, qRgb(75,255,0));
        y += f.m_line_height;
    }

    {
        int x = 0;
        char c[2] = "a";
        pte_font f;
        for (int s = 8; s < 32; s += 2 )
        {
            f = pte_getFont( get_Roboto128(), s );
            x = pte_drawText( &f, x, y, c, 1, qRgb(0,0,0));

            ++ c[0];
        }
        y += f.m_line_height;
    }

    {
        pte_font f = pte_getFont( get_Roboto128(), 24 );

        uint colour = qRgb( 255,0,0 );
        pte_drawText( &f, 5,y, "I WANDERED lonely as a cloud", -1, colour );
        y += f.m_line_height;
        pte_drawText( &f, 5,y, "That floats on high o'er vales and hills,", -1, colour );
        y += f.m_line_height;
        pte_drawText( &f, 5,y, "When all at once I saw a crowd,", -1, colour );
        y += f.m_line_height;
        pte_drawText( &f, 5,y, "A host, of golden daffodils;", -1, colour );
        y += f.m_line_height;
        pte_drawText( &f, 5,y, "Beside the lake, beneath the trees,", -1, colour );
        y += f.m_line_height;
        pte_drawText( &f, 5,y, "Fluttering and dancing in the breeze.", -1, colour );
    }

    // Show what we have created!
    QIcon icon(QPixmap::fromImage((g_image)));
    g_button_icon.swap(icon);
    ui->displayPanel->setIcon(g_button_icon);
    ui->displayPanel->update();
}

MainDlg::~MainDlg()
{
    delete ui;
}



extern "C" void hw_blendPixel( int x, int y, int a, int col )
{
    uint pixel = g_image.pixel(x,y);
    unsigned int c[3];
    unsigned int p[3];

    c[0] = col & 0xff;
    c[1] = (col>>8) & 0xff;
    c[2] = (col>>16) & 0xff;

    p[0] = pixel & 0xff;
    p[1] = (pixel>>8) & 0xff;
    p[2] = (pixel>>16) & 0xff;

    int b = 256 - a;
    p[0] = ((p[0]*b) >> 8) + ((c[2]*a) >> 8);
    p[1] = ((p[1]*b) >> 8) + ((c[1]*a) >> 8);
    p[2] = ((p[2]*b) >> 8) + ((c[0]*a) >> 8);

    pixel = (255<<24)
                | (p[2] << 16)
                | (p[1] << 8)
                | (p[0]);

    g_image.setPixel(x,y,pixel);
}
