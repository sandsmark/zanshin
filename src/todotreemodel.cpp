/* This file is part of Zanshin Todo.

   Copyright 2008 Kevin Ottens <ervin@kde.org>
   Copyright 2008, 2009 Mario Bensi <nef@ipsquad.net>

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

#include "todotreemodel.h"

#include <QtCore/QStringList>

#include <KDE/Akonadi/EntityTreeModel>
#include <KDE/Akonadi/ItemFetchJob>
#include <KDE/Akonadi/ItemMoveJob>
#include <KDE/Akonadi/ItemFetchScope>
#include <KDE/Akonadi/ItemModifyJob>
#include <KDE/Akonadi/TransactionSequence>
#include <KDE/KCalCore/Todo>
#include <KDE/KDebug>
#include <KDE/KIcon>
#include <KDE/KLocale>
#include <KDE/KUrl>


#include <algorithm>
#include <boost/bind.hpp>

#include "kmodelindexproxymapper.h"
#include "todohelpers.h"
#include "todomodel.h"
#include "todonode.h"
#include "todonodemanager.h"

TodoTreeModel::TodoTreeModel(QObject *parent)
    : TodoProxyModelBase(SimpleMapping, parent)
{
}

TodoTreeModel::~TodoTreeModel()
{
}

bool _k_indexLessThan(const QModelIndex &left, const QModelIndex &right)
{
    TodoModel::ItemType leftType = (TodoModel::ItemType) left.data(TodoModel::ItemTypeRole).toInt();
    TodoModel::ItemType rightType = (TodoModel::ItemType) right.data(TodoModel::ItemTypeRole).toInt();

    if (leftType!=rightType) {
        return (leftType==TodoModel::Collection && rightType==TodoModel::ProjectTodo)
            || (leftType==TodoModel::Collection && rightType==TodoModel::StandardTodo)
            || (leftType==TodoModel::ProjectTodo && rightType==TodoModel::StandardTodo);
    }

    if (leftType==TodoModel::Collection) {
        qint64 leftId = left.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();
        qint64 rightId = right.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();

        return leftId<rightId;

    } else if (leftType==TodoModel::ProjectTodo) {
        QStringList leftAncestors = left.data(TodoModel::AncestorsUidRole).toStringList();
        QStringList rightAncestors = right.data(TodoModel::AncestorsUidRole).toStringList();

        return leftAncestors.size()<rightAncestors.size();

    } else if (leftType==TodoModel::StandardTodo) {
        QString leftId = left.data(TodoModel::UidRole).toString();
        QString rightId = right.data(TodoModel::UidRole).toString();
        return leftId<rightId;

    } else {
        kFatal() << "Shouldn't happen, we must get only collections or todos";
        return false;
    }
}

void TodoTreeModel::onSourceInsertRows(const QModelIndex &sourceIndex, int begin, int end)
{
    QList<QModelIndex> sourceChildIndexes;

    // Walking through all the items to harvest the needed info for sorting
    for (int i = begin; i <= end; i++) {
        QModelIndex sourceChildIndex = sourceModel()->index(i, 0, sourceIndex);
        if (sourceChildIndex.isValid()) {
            sourceChildIndexes << sourceChildIndex;
        }
    }

    // Sort, the top level ones first, then one level deep, and so on...
    qSort(sourceChildIndexes.begin(), sourceChildIndexes.end(), &_k_indexLessThan);

    // Now we're sure to add them in the right order, so let's do that!
    TodoNode *collectionNode = m_manager->nodeForSourceIndex(sourceIndex);
    QHash<QString, TodoNode*> uidHash = m_collectionToUidsHash[collectionNode];

    foreach (const QModelIndex &sourceChildIndex, sourceChildIndexes) {
        TodoModel::ItemType type = (TodoModel::ItemType) sourceChildIndex.data(TodoModel::ItemTypeRole).toInt();

        if (type==TodoModel::Collection) {
            //kDebug() << "Adding collection";
            addChildNode(sourceChildIndex, collectionNode);
            onSourceInsertRows(sourceChildIndex, 0, sourceModel()->rowCount(sourceChildIndex)-1);

        } else {
            QString parentUid = sourceChildIndex.data(TodoModel::ParentUidRole).toString();
            TodoNode *parentNode = 0;

            if (uidHash.contains(parentUid)) {
                parentNode = uidHash[parentUid];

            } else {
                if (type==TodoModel::ProjectTodo) {
                    parentNode = collectionNode;
                } else if (type==TodoModel::StandardTodo) {
                    parentNode = m_inboxNode;
                } else {
                    kFatal() << "Shouldn't happen, we must get only collections or todos";
                }
            }

            TodoNode *child = addChildNode(sourceChildIndex, parentNode);
            QString uid = child->data(0, TodoModel::UidRole).toString();
            //kDebug() << "Adding node:" << uid << parentUid;
            uidHash[uid] = child;
        }
    }

    m_collectionToUidsHash[collectionNode] = uidHash;
}

void TodoTreeModel::onSourceRemoveRows(const QModelIndex &sourceIndex, int begin, int end)
{
    for (int i = begin; i <= end; ++i) {
        QModelIndex sourceChildIndex = sourceModel()->index(i, 0, sourceIndex);
        TodoNode *node = m_manager->nodeForSourceIndex(sourceChildIndex);
        if (node) {
            destroyBranch(node);
        }
    }
}

void TodoTreeModel::onSourceDataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    for (int row = begin.row(); row <= end.row(); ++row) {
        QModelIndex sourceChildIndex = sourceModel()->index(row, 0, begin.parent());

        if (!sourceChildIndex.isValid()) {
            continue;
        }

        TodoNode *node = m_manager->nodeForSourceIndex(sourceChildIndex);

        // Collections are just reemited
        if (node->data(0, TodoModel::ItemTypeRole).toInt()==TodoModel::Collection) {
            emit dataChanged(mapFromSource(sourceChildIndex),
                             mapFromSource(sourceChildIndex));
            continue;
        }

        QString oldParentUid = node->parent()->data(0, TodoModel::UidRole).toString();
        QString newParentUid = sourceChildIndex.data(TodoModel::ParentUidRole).toString();

        // If the parent didn't change we just reemit
        if (oldParentUid==newParentUid) {
            emit dataChanged(mapFromSource(sourceChildIndex), mapFromSource(sourceChildIndex));
            continue;
        }

        // The parent did change, so first destroy the old branch...
        destroyBranch(node);

        // Then simulate a row insertion signal
        onSourceInsertRows(sourceChildIndex.parent(), row, row);
    }
}

TodoNode *TodoTreeModel::createInbox() const
{
    TodoNode *node = new TodoNode;

    node->setData(i18n("Inbox"), 0, Qt::DisplayRole);
    node->setData(KIcon("mail-folder-inbox"), 0, Qt::DecorationRole);
    node->setRowData(TodoModel::Inbox, TodoModel::ItemTypeRole);

    return node;
}

void TodoTreeModel::destroyBranch(TodoNode *root)
{
    foreach (TodoNode *child, root->children()) {
        destroyBranch(child);
    }

    QModelIndex proxyParentIndex = m_manager->indexForNode(root->parent(), 0);
    int row = 0;

    if (root->parent()) {
        row = root->parent()->children().indexOf(root);
    } else {
        row = m_manager->roots().indexOf(root);
    }

    beginRemoveRows(proxyParentIndex, row, row);
    m_manager->removeNode(root);
    delete root;
    endRemoveRows();
}

Qt::ItemFlags TodoTreeModel::flags(const QModelIndex &index) const
{
    if (index.data(TodoModel::ItemTypeRole).toInt() == TodoModel::Inbox) {
        return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
    }
    return sourceModel()->flags(mapToSource(index)) | Qt::ItemIsDropEnabled;
}

QMimeData *TodoTreeModel::mimeData(const QModelIndexList &indexes) const
{
    QModelIndexList sourceIndexes;
    foreach (const QModelIndex &proxyIndex, indexes) {
        sourceIndexes << mapToSource(proxyIndex);
    }

    return sourceModel()->mimeData(sourceIndexes);
}

QStringList TodoTreeModel::mimeTypes() const
{
    return sourceModel()->mimeTypes();
}

void TodoTreeModel::moveChildTodo(const QModelIndex &child, const QModelIndex &parent)
{
    if (!child.isValid() || !parent.isValid()) {
        return;
    }

    TodoNode *node = m_manager->nodeForSourceIndex(child);
    if (node)
        destroyBranch(node);

    QModelIndex sourceParentIndex = sourceModel()->index(parent.row(), 0, parent.parent());
    onSourceInsertRows(sourceParentIndex.parent(), child.row(), child.row());

    QModelIndexList children = child.data(TodoModel::ChildIndexesRole).value<QModelIndexList>();
    foreach (const QModelIndex &index, children) {
        Q_ASSERT(index.model()==sourceModel());
        moveChildTodo(index, child);
    }
}

bool TodoTreeModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action,
                                 int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (action != Qt::MoveAction || !KUrl::List::canDecode(mimeData)) {
        return false;
    }

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);

    Akonadi::Collection collection;
    TodoModel::ItemType parentType = (TodoModel::ItemType)parent.data(TodoModel::ItemTypeRole).toInt();
    if (parentType == TodoModel::Collection) {
        collection = parent.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    } else {
        const Akonadi::Item parentItem = parent.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        collection = parentItem.parentCollection();
    }

    QString parentUid = parent.data(TodoModel::UidRole).toString();

    foreach (const KUrl &url, urls) {
        const Akonadi::Item urlItem = Akonadi::Item::fromUrl(url);
        if (urlItem.isValid()) {
            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(urlItem);
            Akonadi::ItemFetchScope scope;
            scope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
            scope.fetchFullPayload();
            job->setFetchScope(scope);

            if ( !job->exec() ) {
                return false;
            }

            foreach (const Akonadi::Item &item, job->items()) {
                if (item.hasPayload<KCalCore::Todo::Ptr>()) {

                    QModelIndexList indexes = Akonadi::EntityTreeModel::modelIndexesForItem(sourceModel(), item);
                    if (indexes.isEmpty()) {
                        return false;
                    }
                    QModelIndex index = indexes.first();
                    TodoHelpers::moveTodoToProject(index, parentUid, parentType, collection);
                    if (index.data(TodoModel::ItemTypeRole).toInt()==TodoModel::ProjectTodo
                     && item.parentCollection().id()==collection.id()) {
                        KModelIndexProxyMapper *mapper = new KModelIndexProxyMapper(index.model(), parent.model(), this);
                        QModelIndex newParent = mapper->mapRightToLeft(parent);
                        moveChildTodo(index, newParent);
                    }
                }
            }
        }
    }

    return true;
}

Qt::DropActions TodoTreeModel::supportedDropActions() const
{
    return sourceModel()->supportedDropActions();
}
