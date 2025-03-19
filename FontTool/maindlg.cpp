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

#include "maindlg.h"
#include "ui_maindlg.h"
#include "fontsampler.h"
#include "selectcharacters.h"
#include <set>
#include <QFontDatabase>
#include <QFileDialog>

MainDlg::MainDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDlg)
{
    ui->setupUi(this);

    // Sort the codec list
    std::set< QByteArray > sorted_codecs;
    foreach (const QByteArray &codec, QTextCodec::availableCodecs())
    {
        sorted_codecs.insert( codec );
    }

    foreach ( const QByteArray &codec, sorted_codecs )
    {
        QString name = QLatin1String( codec );
        ui->charsetSelection->addItem( name );
    }
    ui->charsetSelection->setCurrentText("ISO-8859-1");

    // Now set up the list of fonts that are installed
    QFontDatabase database;
   foreach (const QString &family, database.families())
   {
       ui->fontList->addItem( family );
   }
}

QFont MainDlg::getFont( int size )
{
    QString family = ui->fontList->currentItem()->text();
    QString style = ui->styleSelection->currentText();

    QFontDatabase database;
    QFont font = database.font( family, style, size );
    font.setStyleStrategy(QFont::NoAntialias);
    font.setPixelSize(size);

    return font;
}

void MainDlg::onFontSelected( QListWidgetItem *font )
{
    // Set up the Style dialogue
    QFontDatabase database;
    ui->styleSelection->clear();
    QString family = font->text();
    foreach (const QString styles, database.styles(family))
    {
        ui->styleSelection->addItem( styles );
    }

    ui->label->setFont( getFont(32) );
    ui->label->setText( "The quick brown fox jumped over the lazy dog 01234567890");
}

void MainDlg::onStyleSelected( QString )
{
    QFont displayFont = getFont(32);
    ui->label->setFont( displayFont );
    ui->label->setText( "The quick brown fox jumped over the lazy dog 01234567890");
}


void MainDlg::onSaveFont()
{
    QString fileName = QFileDialog::getSaveFileName(this,
           tr("Save Converted Font File"), "",
           tr("C Files (*.c);;All Files (*)"));

    FontSampler fs;
    fs.m_charSelection = m_charSelection;
    QString codec = ui->charsetSelection->currentText();
    fs.convertFont( getFont( ui->spinBox->value() ), fileName, codec );
}

void MainDlg::onSelectCharacters()
{
    SelectCharacters dlg(this);

    dlg.m_charSelection = m_charSelection;
    dlg.exec();

    if (dlg.result() == Accepted)
    {
        m_charSelection = dlg.m_charSelection;
    }
}

void MainDlg::onClose()
{
    close();
}

MainDlg::~MainDlg()
{
    delete ui;
}
