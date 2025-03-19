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
#ifndef MAINDLG_H
#define MAINDLG_H

#include <QDialog>
#include <QListWidget>
#include "selectcharacters.h"

namespace Ui {
class MainDlg;
}

class MainDlg : public QDialog
{
    Q_OBJECT

public:
    explicit MainDlg(QWidget *parent = 0);
    ~MainDlg();

public slots:
    void onFontSelected( QListWidgetItem *i1 );
    void onStyleSelected( QString style );
    QFont getFont( int size );
    void onSaveFont();
    void onClose();
    void onSelectCharacters();

private:
    Ui::MainDlg *ui;

    std::vector<intPair>        m_charSelection;

};

#endif // MAINDLG_H
