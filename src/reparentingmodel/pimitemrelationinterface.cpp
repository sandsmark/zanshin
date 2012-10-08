/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pimitemrelationinterface.h"

#include <QtCore/QAbstractItemModel>

#include <KDE/Akonadi/ItemModifyJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/TransactionSequence>
#include <Akonadi/ItemDeleteJob>
#include <KDE/KCalCore/Todo>
#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KLocale>
#include <KDE/KMessageBox>
#include <KUrl>

#include "globaldefs.h"
#include "todohelpers.h"
#include <pimitem.h>
#include <note.h>

K_GLOBAL_STATIC(PimItemRelationInterface, s_contextManager)
K_GLOBAL_STATIC(PimItemRelationInterface, s_topicManager)
K_GLOBAL_STATIC(ProjectStructureInterface, s_projectManager)

PimNode PimItemStructureInterface::fromIndex(const QModelIndex &index)
{
     Zanshin::ItemType itemType = (Zanshin::ItemType)index.data(Zanshin::ItemTypeRole).toInt();
     kDebug() << index << itemType;
     switch (itemType) {
         case Zanshin::Collection: {
             PimNode node(PimNode::Collection);
             node.collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
             return node;
         }
         case Zanshin::ProjectTodo: {
             PimNode node (PimNode::Project);
             node.item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
             node.uid = index.data(Zanshin::UidRole).toString();
             return node;
         }
         case Zanshin::Category:
         case Zanshin::CategoryRoot: {
             PimNode node (PimNode::Context);
             node.relationId = index.data(Zanshin::RelationIdRole).value<Id>();
             node.uid = index.data(Zanshin::UidRole).toString();
             return node;
         }
         case Zanshin::Topic:
         case Zanshin::TopicRoot: {
             PimNode node (PimNode::Topic);
             node.relationId = index.data(Zanshin::RelationIdRole).value<Id>();
             node.uid = index.data(Zanshin::UidRole).toString();
             return node;
         }
         case Zanshin::StandardTodo: {
             PimNode node (PimNode::PimItem);
             node.item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
             node.uid = index.data(Zanshin::UidRole).toString();
             return node;
         }
     }
     Q_ASSERT(false);
     return PimNode(PimNode::Invalid);
}

// PimNode PimItemStructureInterface::projectNode(const Akonadi::Item& )
// {
// 
// }
// 
// PimNode PimItemStructureInterface::contextNode(Id )
// {
// 
// }
// 
// PimNode PimItemStructureInterface::topicNode(Id )
// {
// 
// }
// 
// PimNode PimItemStructureInterface::todoNode(const Akonadi::Item& )
// {
// 
// }
// 
// PimNode PimItemStructureInterface::noteNode(const Akonadi::Item& )
// {
// 
// }
// 
// PimNode PimItemStructureInterface::collectionNode(const Akonadi::Collection& )
// {
// 
// }
// 

void PimItemStructureInterface::create(PimNode::NodeType type, const QString& name, const QList< PimNode >& parents, const Akonadi::Collection& col)
{
    QList<PimItemRelation> relations;
    if (!parents.isEmpty()) {
        const PimNode parent = parents.first();
        kDebug() << "relation " << parent.uid;
        if (parent.type == PimNode::Project) {
            relations << PimItemRelation(PimItemRelation::Project, QList<PimItemTreeNode>() << PimItemTreeNode(parent.uid.toLatin1()));
        }
        if (parent.type == PimNode::Context) {
            relations << PimItemRelation(PimItemRelation::Context, QList<PimItemTreeNode>() << PimItemTreeNode(parent.uid.toLatin1()));
        }
    }
    switch (type) {
        case PimNode::Project:
            kDebug() << "adding project: " << name << col.url().url();
            TodoHelpers::addTodo(name, relations, col, true);
            break;
        case PimNode::Todo: {
            kDebug() << "adding todo: " << name << col.url().url();
            TodoHelpers::addTodo(name, relations, col, false);
            break;
        }
        case PimNode::Note: {
            Note note;
            note.setTitle(name);
            Akonadi::ItemCreateJob *itemCreateJob = new Akonadi::ItemCreateJob(note.getItem(), col);
            break;
        }
        case PimNode::Context:
            PimItemStructureInterface::contextInstance().add(name, parents);
            break;
        case PimNode::Topic:
            PimItemStructureInterface::topicInstance().add(name, parents);
            break;
        default:
            Q_ASSERT(0);
    }
}

void PimItemStructureInterface::remove(const PimNode& node, QWidget *parent)
{
    switch (node.type) {
        case PimNode::Project:
            PimItemStructureInterface::projectInstance().remove(node, parent);
            break;
        case PimNode::Todo:
        case PimNode::Note:
        case PimNode::PimItem:
            new Akonadi::ItemDeleteJob(node.item);
            break;
        case PimNode::Context:
            PimItemStructureInterface::contextInstance().remove(node, parent);
            break;
        case PimNode::Topic:
            PimItemStructureInterface::topicInstance().remove(node, parent);
            break;
        default:
            Q_ASSERT(0);
    }
}

void PimItemStructureInterface::remove(const QList< PimNode >& nodes, QWidget *parent)
{
    foreach(const PimNode &node, nodes) {
        remove(node, parent);
    }
}

void PimItemStructureInterface::moveTo(const PimNode& node, const PimNode& parent)
{

}

void PimItemStructureInterface::linkTo(const PimNode& node, const PimNode& parent)
{

}

void PimItemStructureInterface::unlink(const PimNode& node, const PimNode& parent)
{

}

void PimItemStructureInterface::rename(const PimNode& node, const QString& name)
{

}


PimItemStructureInterface &PimItemStructureInterface::contextInstance()
{
    Q_ASSERT(s_contextManager);
    return *s_contextManager;
}

PimItemStructureInterface &PimItemStructureInterface::topicInstance()
{
    Q_ASSERT(s_topicManager);
    return *s_topicManager;
}

ProjectStructureInterface& PimItemStructureInterface::projectInstance()
{
    Q_ASSERT(s_projectManager);
    return *s_projectManager;
}

void PimItemStructureInterface::setRelationsStructure(PimItemRelationCache *s)
{
    mStructure = s;
}




PimItemRelationInterface::PimItemRelationInterface()
{
}

PimItemRelationInterface::~PimItemRelationInterface()
{
}

static Id toId(const PimNode &index)
{
    return index.relationId;
}

static IdList toId(const QList<PimNode> &list)
{
    IdList parentIds;
    foreach (const PimNode &index, list) {
        parentIds << index.relationId;
    }
    return parentIds;
}

void PimItemRelationInterface::add(const QString& name, const QList<PimNode>& parents)
{
    kDebug() << name << toId(parents);
    mStructure->addNode(name, toId(parents));
}

// bool PimItemRelationInterface::remove(QWidget* widget, const QModelIndexList& relations)
// {
//     IdList relationIds = toId(relations);
//     if (widget) {
//         QStringList categoryList;
//         foreach (Id category, relationIds) {
//             categoryList << mStructure->getName(category);
//         }
//         QString categoryName = categoryList.join(", ");
//         kDebug() << relationIds << categoryList;
//         QString title;
//         QString text;
//         //TODO adjust Context/Topic
//         if (relationIds.size() > 1) {
//             text = i18n("Do you really want to delete the context '%1'? All actions won't be associated to this context anymore.", categoryName);
//             title = i18n("Delete Context");
//         } else {
//             text = i18n("Do you really want to delete the contexts '%1'? All actions won't be associated to those contexts anymore.", categoryName);
//             title = i18n("Delete Contexts");
//         }
//         int button = KMessageBox::questionYesNo(widget, text, title);
//         bool canRemove = (button==KMessageBox::Yes);
// 
//         if (!canRemove) {
//             return false;
//         }
//     }
//     kDebug() << "remove " << relationIds;
//     foreach (Id id, relationIds) {
//         mStructure->removeNode(id);
//     }
//     return true;
// }
// 
// bool PimItemRelationInterface::remove(const Id& relation)
// {
//     kDebug() << relation;
//     mStructure->removeNode(relation);
//     return true;
// }
// 
// bool PimItemRelationInterface::moveTo(const QModelIndex& node, const QModelIndex& parent)
// {
//     Zanshin::ItemType parentType = (Zanshin::ItemType)parent.data(Zanshin::ItemTypeRole).toInt();
//     Id id = toId(node);
//     Id parentId = toId(parent);
//     kDebug() << id << parentId;
//     if (parentType!=Zanshin::Category && parentType!=Zanshin::CategoryRoot) { //TODO shouldn't be necessary
//         return false;
//     }
//     mStructure->moveNode(id, IdList() << parentId);
//     return true;
// }
// 
// bool PimItemRelationInterface::linkTo(const QModelIndex& node, const QModelIndex& parent)
// {
//     Id id = toId(node);
//     Id parentId = toId(parent);
//     kDebug() << id << parentId;
// //     if (parentType!=Zanshin::Category && parentType!=Zanshin::CategoryRoot) { //TODO shouldn't be necessary
// //         return false;
// //     }
//     IdList parents = mStructure->getParents(id);
//     parents << parentId;
//     mStructure->moveNode(id, parents);
//     return true;
// }
// 
// 
// bool PimItemRelationInterface::unlink(const Akonadi::Item& item, const QModelIndex& parent)
// {
//     Id parentId = toId(parent);
//     kDebug() << item.id() << parentId;
//     if (!item.isValid()) {
//         return false;
//     }
//     Id id = mStructure->getItemId(item);
//     IdList parents = mStructure->getParents(id);
//     parents.removeAll(parentId);
//     mStructure->moveNode(id, parents);
//     return true;
// }
// 
// bool PimItemRelationInterface::rename(const QModelIndex& node, const QString& name)
// {
//     mStructure->renameNode(toId(node), name);
//     return true;
// }
// 
// 
// // IdList PimItemRelationInterface::getParents(const Akonadi::Item& item) const
// // {
// //     if (!item.isValid()) {
// //         return IdList();
// //     }
// //     Id id = mStructure->getItemId(item);
// //     return mStructure->getParents(id);
// // }
// // 
// // IdList PimItemRelationInterface::getAncestors(Id id) const
// // {
// //     IdList ancestors;
// //     foreach (Id parent, mStructure->getParents(id)) {
// //         kDebug() << mStructure->getPath(parent) << parent;
// //         ancestors << parent;
// //         ancestors << getAncestors(parent);
// //     }
// //     return ancestors;
// // }
// // 
// // IdList PimItemRelationInterface::getAncestors(const Akonadi::Item& item) const
// // {
// //     if (!item.isValid()) {
// //         return IdList();
// //     }
// //     IdList ancestors;
// //     Id id = mStructure->getItemId(item);
// //     kDebug() << "ancestors of " << mStructure->getPath(id) << id;
// //     return getAncestors(id);
// // }
//
// const char *todoScheme = "Todo";
// static PimNode nodeUri(PimItemStructureInterface::NodeType type, const QString &uid)
// {
//     PimNode uri;
//     switch (type) {
//         case PimItemStructureInterface::Todo:
//             uri.setScheme(todoScheme);
//             break;
//     }
//     uri.setPath(uid);
//     return uri;
// }
// 
// static PimItemStructureInterface::NodeType getNodeType(const PimNode &uri)
// {
//     if (uri.scheme() == todoScheme) {
//         return PimItemStructureInterface::Todo;
//     }
//     return PimItemStructureInterface::Invalid;
// }

ProjectStructureInterface::ProjectStructureInterface()
{

}

bool ProjectStructureInterface::moveTo(const PimNode& node, const PimNode& parent)
{
    PimNode::NodeType nodeType = node.type;
    PimNode::NodeType parentType = parent.type;
//    const Akonadi::Item item = node.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();

    if (nodeType == PimNode::Invalid || parentType == PimNode::Invalid) {
        return false;
    }

    IdList parents;
    if (parent.type != PimNode::Empty) {
        parents << mStructure->getId(parent.uid.toLatin1());
    }
    Id nodeId = mStructure->getItemId(node.item);
    mStructure->moveNode(nodeId, parents);
    return true;

//     if ((itemType == Zanshin::StandardTodo && parentType == Zanshin::StandardTodo)
//      || (itemType == Zanshin::ProjectTodo && parentType == Zanshin::StandardTodo)
//      || (itemType == Zanshin::Collection && parentType == Zanshin::ProjectTodo)
//      || (itemType == Zanshin::Collection && parentType == Zanshin::StandardTodo)) {
//          return false;
//     }

//     const Akonadi::Item parentItem = parent.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
//     QScopedPointer<AbstractPimItem> pimitem(PimItemUtils::getItem(parentItem));
//     Q_ASSERT(pimitem);
//     Akonadi::Collection parentCollection;
// 
//     return TodoHelpers::moveTodoToProject(item, pimitem->getUid(), parentType, parentCollection);
}

void ProjectStructureInterface::remove(const QList< PimNode >& nodes, QWidget *parent)
{

    if (nodes.isEmpty()) {
        return;
    }

    bool canRemove = true;
    QString summary;
    IdList projectList;
    if (nodes.size() > 1) {
//         QStringList projectList;
//         foreach (QModelIndex project, projects) {
//             projectList << project.data().toString();
//         }
//         summary = projectList.join(", ");
    } else {
//         QModelIndexList children = projects[0].data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
//         if (!children.isEmpty()) {
//             summary = projects[0].data().toString();
//         }
    }

    if (!summary.isEmpty()) {
        QString title;
        QString text;

//         if (projects.size() > 1) {
//             title = i18n("Delete Projects");
//             text = i18n("Do you really want to delete the projects '%1', with all its actions?", summary);
//         } else {
            title = i18n("Delete Project");
            text = i18n("Do you really want to delete the project '%1', with all its actions?", summary);
//         }

        int button = KMessageBox::questionYesNo(parent, text, title);
        canRemove = (button==KMessageBox::Yes);
    }

    if (!canRemove) return;

    Akonadi::TransactionSequence *sequence = new Akonadi::TransactionSequence();
    foreach (const PimNode &node, nodes) {
        Q_ASSERT(node.item.isValid());
        Id id = mStructure->getItemId(node.item);
        IdList children = static_cast<ProjectStructure*>(mStructure.data())->getChildren(id);
        foreach (Id child, children) {
            Akonadi::Item item(static_cast<ProjectStructure*>(mStructure.data())->itemId(child));
            kDebug() << "remove " << item.id();
//             new Akonadi::ItemDeleteJob(item, sequence);
        }
            kDebug() << "remove " << node.item.id();
//         new Akonadi::ItemDeleteJob(node.item, sequence);
    }
//     sequence->start();
    return;
}

void ProjectStructureInterface::remove(const PimNode& node, QWidget *parent)
{
    QList<PimNode> projects;
    projects << node;
    return remove(projects, parent);
}



