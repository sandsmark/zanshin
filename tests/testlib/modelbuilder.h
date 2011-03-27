/* This file is part of Zanshin Todo.

   Copyright 2011 Kevin Ottens <ervin@kde.org>

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

#ifndef ZANSHIN_TESTLIB_MODELBUILDER_H
#define ZANSHIN_TESTLIB_MODELBUILDER_H

#include <QtCore/QList>
#include <QtGui/QStandardItemModel>
#include <KDE/Akonadi/EntityTreeModel>

#include <testlib/modelpath.h>
#include <testlib/modelstructure.h>

namespace Zanshin
{
namespace Test
{

enum Roles {
    TestDslRole = Akonadi::EntityTreeModel::UserRole + 101,
    UserRole = Akonadi::EntityTreeModel::UserRole + 200
};

class ModelBuilderBehaviorBase;

class ModelBuilder
{
public:
    ModelBuilder();

    QStandardItemModel *model() const;
    void setModel(QStandardItemModel *model);

    ModelBuilderBehaviorBase *behavior() const;
    void setBehavior(ModelBuilderBehaviorBase *behavior);

    void create(const ModelStructure &structure, const ModelPath &root = ModelPath());

private:
    QStandardItem *locateItem(const ModelPath &root);
    QList< QList<QStandardItem*> > createItems(const ModelStructure &structure);
    QList<QStandardItem*> createItem(const ModelStructureTreeNode *node);

    QStandardItemModel *m_model;
    ModelBuilderBehaviorBase *m_behavior;
};

} // namespace Test
} // namespace Zanshin

#endif
