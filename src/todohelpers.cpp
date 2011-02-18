/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>
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

#include "todohelpers.h"

#include <KDE/Akonadi/Collection>
#include <KDE/Akonadi/Item>
#include <KDE/Akonadi/ItemCreateJob>
#include <KDE/Akonadi/ItemDeleteJob>
#include <KDE/Akonadi/ItemModifyJob>
#include <KDE/Akonadi/ItemMoveJob>
#include <KDE/Akonadi/EntityTreeModel>
#include <KDE/KCalCore/Todo>
#include <KDE/KLocale>
#include <KDE/KMessageBox>

#include "categorymanager.h"
#include "globaldefs.h"

void TodoHelpers::addProject(const QString &summary, const Akonadi::Collection &collection)
{
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void TodoHelpers::addProject(const QString &summary, const Akonadi::Item &parentProject)
{
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    KCalCore::Todo::Ptr parentTodo = parentProject.payload<KCalCore::Todo::Ptr>();
    todo->setRelatedTo(parentTodo->uid());

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::Collection collection = parentProject.parentCollection();

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void TodoHelpers::addCategory(const QString &category, const QString &parentCategory)
{
    QString categoryPath;
    if (parentCategory.isEmpty()) {
        categoryPath = category;
    } else {
        categoryPath = parentCategory + CategoryManager::pathSeparator() + category;
    }
    CategoryManager::instance().addCategory(categoryPath);
}

void removeCurrentTodo(const QModelIndex &project, QModelIndexList children, Akonadi::TransactionSequence *sequence)
{
    foreach (QModelIndex child, children) {
        QModelIndexList childList = child.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        removeCurrentTodo(child, childList, sequence);
    }

    Akonadi::Item item = project.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    new Akonadi::ItemDeleteJob(item, sequence);
}

bool TodoHelpers::removeProject(QWidget *parent, const QModelIndex &project)
{
    bool canRemove = true;
    QModelIndexList children = project.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
    if (!children.isEmpty()) {
        QString summary = project.data().toString();

        QString title;
        QString text;

        text = i18n("Do you really want to delete the project '%1', with all its actions?", summary);
        title = i18n("Delete Project");

        int button = KMessageBox::questionYesNo(parent, text, title);
        canRemove = (button==KMessageBox::Yes);
    }

    if (!canRemove) return false;

    Akonadi::TransactionSequence *sequence = new Akonadi::TransactionSequence();
    removeCurrentTodo(project, children, sequence);
    sequence->start();
    return true;
}

bool TodoHelpers::removeCategory(QWidget *parent, const QModelIndex &categoryIndex)
{
    QString category = categoryIndex.data().toString();
    QString path = categoryIndex.data(Zanshin::CategoryPathRole).toString();
    QString title;
    QString text;

    text = i18n("Do you really want to delete the category '%1'? All actions won't be associated to those contexts anymore.", category);
    title = i18n("Delete Category");

    int button = KMessageBox::questionYesNo(parent, text, title);
    bool canRemove = (button==KMessageBox::Yes);

    if (!canRemove) return false;

    return CategoryManager::instance().removeCategory(path);
}

bool TodoHelpers::removeTodoFromCategory(const QModelIndex &index, const QString &category)
{
    return CategoryManager::instance().removeTodoFromCategory(index, category);
}

void changeCollection(const Akonadi::Item &item, QModelIndexList children, const Akonadi::Collection &parentCollection, Akonadi::TransactionSequence *sequence)
{
    new Akonadi::ItemMoveJob(item, parentCollection, sequence);
    foreach (QModelIndex child, children) {
        QModelIndexList childList = child.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        Akonadi::Item item = child.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        changeCollection(item, childList, parentCollection, sequence);
    }
}

bool TodoHelpers::moveTodoToProject(const QModelIndex &index, const QString &parentUid, const Zanshin::ItemType parentType, const Akonadi::Collection &parentCollection)
{
    Zanshin::ItemType itemType = (Zanshin::ItemType)index.data(Zanshin::ItemTypeRole).toInt();
    const Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();

    if (!todo) {
        return false;
    }

    if ((todo->relatedTo() == parentUid)
     || (itemType == Zanshin::StandardTodo && parentType == Zanshin::StandardTodo)
     || (itemType == Zanshin::ProjectTodo && parentType == Zanshin::StandardTodo)
     || (itemType == Zanshin::Collection && parentType == Zanshin::ProjectTodo)
     || (itemType == Zanshin::Collection && parentType == Zanshin::StandardTodo)) {
         return false;
    }

    if (parentType == Zanshin::Inbox || parentType == Zanshin::Collection) {
        todo->setRelatedTo("");
    } else {
        todo->setRelatedTo(parentUid);
    }

    Akonadi::TransactionSequence *transaction = new Akonadi::TransactionSequence();

    new Akonadi::ItemModifyJob(item, transaction);

    int itemCollectonId = item.parentCollection().id();
    int parentCollectionId = parentCollection.id();
    if ((parentType != Zanshin::Inbox) && (itemCollectonId != parentCollectionId)) {
        QModelIndexList childList = index.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
        changeCollection(item, childList, parentCollection, transaction);
    }
    return true;
}

bool TodoHelpers::moveTodoToCategory(const QModelIndex &index, const QString &category, const Zanshin::ItemType parentType)
{
    return CategoryManager::instance().moveTodoToCategory(index, category, parentType);
}

void TodoHelpers::moveCategory(const QString &oldCategoryPath, const QString &parentPath, const Zanshin::ItemType parentType)
{
    if (parentType!=Zanshin::Category && parentType!=Zanshin::CategoryRoot) {
        return;
    }

    QString categoryName = oldCategoryPath.split(CategoryManager::pathSeparator()).last();
    QString newCategoryPath;
    if (parentType==Zanshin::Category) {
        newCategoryPath = parentPath + CategoryManager::pathSeparator() + categoryName;
    } else {
        newCategoryPath = categoryName;
    }

    CategoryManager::instance().moveCategory(oldCategoryPath, newCategoryPath);
}

void TodoHelpers::renameCategory(const QString &oldCategoryPath, const QString &newCategoryName)
{
    QString newCategoryPath = oldCategoryPath.left(oldCategoryPath.lastIndexOf(CategoryManager::pathSeparator())+1) + newCategoryName;
    CategoryManager::instance().renameCategory(oldCategoryPath, newCategoryPath);
}
