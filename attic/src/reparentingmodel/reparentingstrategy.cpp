/*
    This file is part of Zanshin Todo.

    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "reparentingstrategy.h"
#include "globaldefs.h"
#include "reparentingmodel.h"
#include "todonode.h"
#include <klocalizedstring.h>
#include <KIcon>


ReparentingStrategy::ReparentingStrategy()
:   mReparentOnRemoval(true),
    m_model(0),
    mMinIdCounter(10),
    mIdCounter(mMinIdCounter)
{

}


IdList ReparentingStrategy::getParents(const QModelIndex &, const IdList & )
{
    return IdList();
}

void ReparentingStrategy::onNodeRemoval(const qint64& /*changed*/)
{

}

void ReparentingStrategy::setModel(ReparentingModel* model)
{
    m_model = model;
}

Id ReparentingStrategy::getNextId()
{
    return mIdCounter++;
}

void ReparentingStrategy::setMinId(Id id)
{
    mMinIdCounter = id;
}

void ReparentingStrategy::reset()
{
    mIdCounter = mMinIdCounter;
}


QList<TodoNode*> ReparentingStrategy::createNode(Id id, IdList parents, QString name)
{
//     kDebug() << id << parents << name;
    return m_model->createNode(id, parents, name);
}

void ReparentingStrategy::renameNode(Id id, QString name)
{
//     kDebug() << id << name;
    m_model->renameNode(id, name);
}

void ReparentingStrategy::updateParents(Id id, IdList parents)
{
//     kDebug() << id << parents;
    m_model->reparentNode(id, parents);
}

void ReparentingStrategy::updateParents(Id id)
{
    TodoNode *node = m_model->m_parentMap.leftToRight(id);
    if (!node || !node->rowSourceIndex().isValid()) {
//         kDebug() << id;
        return;
    }
    Q_ASSERT(node);
    IdList parents = getParents(node->rowSourceIndex());
//     kDebug() << id << parents;
    m_model->reparentNode(id, parents, node->rowSourceIndex());
}


void ReparentingStrategy::removeNode(Id id)
{
//     kDebug() << id;
    m_model->removeNodeById(id);
}


bool ReparentingStrategy::reparentOnParentRemoval(Id) const
{
    return mReparentOnRemoval;
}

QVariant ReparentingStrategy::getData(Id id, int role) const
{
    TodoNode *node = m_model->m_parentMap.leftToRight(id);
    Q_ASSERT(node);
    return node->data(0, role);
}


void ReparentingStrategy::setData(Id id, const QVariant& value, int role)
{
    TodoNode *node = m_model->m_parentMap.leftToRight(id);
    Q_ASSERT(node);
    node->setData(value, 0, role);
}

Akonadi::Collection ReparentingStrategy::getParentCollection(Id id)
{

    TodoNode *node = m_model->m_parentMap.leftToRight(id);
    Q_ASSERT(node);
    return node->rowSourceIndex().parent().data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}



TestReparentingStrategy::TestReparentingStrategy()
: ReparentingStrategy()
{

}

Id TestReparentingStrategy::getId(const QModelIndex &sourceChildIndex)
{
    if (!sourceChildIndex.data(IdRole).isValid()) {
        kWarning() << "error: missing idRole";
    }
    return sourceChildIndex.data(IdRole).value<Id>();
}

IdList TestReparentingStrategy::getParents(const QModelIndex &sourceChildIndex, const IdList &ignore)
{
    if (!sourceChildIndex.isValid()) {
        kWarning() << "invalid index";
        return IdList();
    }

    if (sourceChildIndex.data(ParentListRole).canConvert<IdList>()) {
        IdList list = sourceChildIndex.data(ParentListRole).value<IdList>();
        foreach (Id toRemove, ignore) {
            list.removeAll(toRemove);
        }
        return list;
    }

    if (!sourceChildIndex.data(ParentRole).isValid()) {
        return IdList();
    }
    const Id &parent = sourceChildIndex.data(ParentRole).value<Id>();
    if (parent < 0) {
        return IdList();
    }
    if (ignore.contains(parent)) {
        return IdList();
    }

    return IdList() << parent;
}




TestParentStructureStrategy::TestParentStructureStrategy()
: ReparentingStrategy()
{
}

void TestParentStructureStrategy::init()
{
    ReparentingStrategy::init();
    QList<TodoNode*> nodes = createNode(997, IdList(), "No Topic");
    Q_ASSERT(nodes.size() == 1);
    TodoNode *node = nodes.first();
    node->setData(i18n("No Topic"), 0, Qt::DisplayRole);
    node->setData(KIcon("mail-folder-inbox"), 0, Qt::DecorationRole);
    node->setRowData(Zanshin::Inbox, Zanshin::ItemTypeRole);

    QList<TodoNode*> nodes2 = createNode(998, IdList(), "Topics");
    Q_ASSERT(nodes2.size() == 1);
    TodoNode *node2 = nodes2.first();
    node2->setData(i18n("Topics"), 0, Qt::DisplayRole);
    node2->setData(KIcon("document-multiple"), 0, Qt::DecorationRole);
    node2->setRowData(Zanshin::TopicRoot, Zanshin::ItemTypeRole);
}

bool TestParentStructureStrategy::reparentOnParentRemoval(Id id) const
{
    if (id < 900) {
        kDebug() << "reparent " << id;
        return false;
    }
    return true;
}


Id TestParentStructureStrategy::getId(const QModelIndex &sourceChildIndex)
{
    if (!sourceChildIndex.isValid()) {
        kWarning() << "invalid index";
        return -1;
    }
    // modelutils.h IdRole=102
    return sourceChildIndex.data(102).value<qint64>()+1000;
}

IdList TestParentStructureStrategy::getParents(const QModelIndex &sourceChildIndex, const IdList &ignore)
{
    Q_ASSERT(sourceChildIndex.isValid());

    if (!sourceChildIndex.data(TopicParentRole).isValid()) {
        return IdList() << 997; //No Topics
    }
    const Id &parent = sourceChildIndex.data(TopicParentRole).value<Id>();
    if (ignore.contains(parent)) {
        return IdList() << 997; //No Topics
    }
    return IdList() << parent;
}

void TestParentStructureStrategy::addParent(Id identifier, Id parentIdentifier, const QString& name)
{
    kDebug() << identifier << parentIdentifier << name;
    if (parentIdentifier < 0 ) {
        parentIdentifier = 998;
    }
    createNode(identifier, IdList() << parentIdentifier, name);
}

void TestParentStructureStrategy::setParent(const QModelIndex &item, const qint64& parentIdentifier)
{
    updateParents(getId(item), IdList() << parentIdentifier);
}


void TestParentStructureStrategy::removeParent(const Id& identifier)
{
    removeNode(identifier);
}



