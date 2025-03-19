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

#ifndef SELECTCHARACTERS_H
#define SELECTCHARACTERS_H

#include <QDialog>

typedef std::pair<int,int>  intPair;

namespace Ui {
class SelectCharacters;
}

class SelectCharacters : public QDialog
{
    Q_OBJECT

public:
    explicit SelectCharacters(QWidget *parent = 0);
    ~SelectCharacters();

    // Here is the data we are editing
    std::vector<intPair>        m_charSelection;

public slots:
    void onDeleteSel();
    void onAddSel();

private:
    Ui::SelectCharacters *ui;

    void showEvent(QShowEvent * event);
    void updateSelection();
};

#endif // SELECTCHARACTERS_H
