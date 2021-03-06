/* This file is part of Zanshin Todo.

   Copyright 2008-2009 Kevin Ottens <ervin@kde.org>

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

#include "sidebarpage.h"

#include <KDE/KDebug>
#include <KDE/KInputDialog>
#include <KDE/KLocale>
#include <KDE/KMessageBox>
#include <KJob>

#include <QtGui/QVBoxLayout>
#include <QtGui/QHeaderView>

#include "globaldefs.h"
#include "todohelpers.h"
#include "gui/shared/todotreeview.h"
#include "core/pimitemservices.h"

SideBarPage::SideBarPage(QAbstractItemModel *model,
                         QItemSelectionModel *selectionModel,
                         const QList<QAction*> &contextActions,
                         QWidget *parent)
    : QWidget(parent)
{
    setLayout(new QVBoxLayout(this));
    m_treeView = new TodoTreeView(this);
    layout()->addWidget(m_treeView);
    layout()->setContentsMargins(0, 0, 0, 0);

    m_treeView->setFocusPolicy(Qt::NoFocus);
    m_treeView->header()->hide();
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setAnimated(true);
    m_treeView->setModel(model);
    QItemSelectionModel *oldSelectionModel = m_treeView->selectionModel();
    m_treeView->setSelectionModel(selectionModel);
    oldSelectionModel->deleteLater();
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setDragEnabled(true);
    m_treeView->viewport()->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setStyleSheet("QTreeView { background: transparent; border-style: none; }");

    m_treeView->setCurrentIndex(m_treeView->model()->index(0, 0));

    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_treeView, SLOT(expand(QModelIndex)));

    m_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_treeView->addActions(contextActions);
}

QItemSelectionModel *SideBarPage::selectionModel() const
{
    return m_treeView->selectionModel();
}

void SideBarPage::addNewItem()
{
    QModelIndex parentItem = selectionModel()->currentIndex();
    Zanshin::ItemType type = (Zanshin::ItemType) parentItem.data(Zanshin::ItemTypeRole).toInt();

    QString title;
    QString text;

    if (type==Zanshin::Collection
     || type==Zanshin::ProjectTodo) {
        title = i18n("New Project");
        text = i18n("Enter project name:");

    } else if (type==Zanshin::ContextRoot
            || type==Zanshin::Context) {
        title = i18n("New Context");
        text = i18n("Enter context name:");

    } else if (type==Zanshin::TopicRoot
            || type==Zanshin::Topic) {
        title = i18n("New Topic");
        text = i18n("Enter topic name:");

    } else {
        kFatal() << "We should never, ever, get in this case...";
    }


    bool ok;
    QString summary = KInputDialog::getText(title, text,
                                            QString(), &ok, this);
    summary = summary.trimmed();

    if (!ok || summary.isEmpty()) return;

    if (type==Zanshin::Collection) {
        Akonadi::Collection collection = parentItem.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        PimItemServices::create(PimItem::Project, summary, QList<PimItem::Ptr>(), collection);
    } else if (type==Zanshin::ProjectTodo) {
        Akonadi::Collection collection = parentItem.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
        PimItemServices::create(PimItem::Project, summary, QList<PimItem::Ptr>() << PimItemServices::fromIndex(parentItem), collection);
    } else if (type==Zanshin::ContextRoot) {
        PimItemServices::create(PimItem::Context, summary);
    } else if (type==Zanshin::Context) {
        PimItemServices::create(PimItem::Context, summary, QList<PimItem::Ptr>() << PimItemServices::fromIndex(parentItem));
    } else if (type==Zanshin::TopicRoot) {
        PimItemServices::create(PimItem::Topic, summary);
    } else if (type==Zanshin::Topic) {
        PimItemServices::create(PimItem::Topic, summary, QList<PimItem::Ptr>() << PimItemServices::fromIndex(parentItem));
    } else {
        kFatal() << "We should never, ever, get in this case...";
    }
}

void SideBarPage::result(KJob *job)
{
    kDebug() << job->error() << job->errorString();
}


void SideBarPage::removeCurrentItem()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid()) {
        return;
    }
    PimItemServices::remove(PimItemServices::fromIndex(current), this);
    //TODO only if remove succeeded
    m_treeView->setCurrentIndex(current.parent());
}

void SideBarPage::renameCurrentItem()
{
    m_treeView->edit(selectionModel()->currentIndex());
}

void SideBarPage::selectPreviousItem()
{
    QModelIndex index = m_treeView->currentIndex();
    index = m_treeView->indexAbove(index);

    if (index.isValid()) {
        m_treeView->setCurrentIndex(index);
    }
}

void SideBarPage::selectNextItem()
{
    QModelIndex index = m_treeView->currentIndex();
    m_treeView->expand(index);
    index = m_treeView->indexBelow(index);

    if (index.isValid()) {
        m_treeView->setCurrentIndex(index);
    }
}
