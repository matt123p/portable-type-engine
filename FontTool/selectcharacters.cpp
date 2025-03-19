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

#include "selectcharacters.h"
#include "ui_selectcharacters.h"

SelectCharacters::SelectCharacters(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectCharacters)
{
    ui->setupUi(this);
}

void SelectCharacters::showEvent(QShowEvent *)
{
    updateSelection();
}


void SelectCharacters::updateSelection()
{
    ui->selectionList->clear();
    foreach (const intPair p, m_charSelection)
    {
        QString s = QString("%1 to %2").arg( p.first ).arg( p.second );
        ui->selectionList->addItem(s);
    }
}

SelectCharacters::~SelectCharacters()
{
    delete ui;
}

void SelectCharacters::onDeleteSel()
{
    int offset = 0;
    foreach (QModelIndex sel, ui->selectionList->selectionModel()->selectedIndexes())
    {
        m_charSelection.erase( m_charSelection.begin() + sel.row() - offset );
        ++ offset;
    }

    updateSelection();
}

void SelectCharacters::onAddSel()
{
    intPair p;
    p.first = ui->minValue->value();
    p.second = ui->maxValue->value();
    m_charSelection.push_back( p );

    updateSelection();
}
