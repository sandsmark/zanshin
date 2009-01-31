/* This file is part of Zanshin Todo.

   Copyright 2008 Kevin Ottens <ervin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#ifndef ZANSHIN_ACTIONLISTDELEGATE_H
#define ZANSHIN_ACTIONLISTDELEGATE_H

#include <QtGui/QStyledItemDelegate>

#include "todoflatmodel.h"

class ActionListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ActionListDelegate(QObject *parent = 0);
    virtual ~ActionListDelegate();

    void setDragModeCount(int count);

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    virtual void updateEditorGeometry(QWidget *editor,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const;

    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index);

private:
    TodoFlatModel::ItemType rowType(const QModelIndex &index) const;
    bool isInFocus(const QModelIndex &index) const;
    bool isCompleted(const QModelIndex &index) const;
    bool isOverdue(const QModelIndex &index) const;

    mutable int m_dragModeCount;
};
#endif
