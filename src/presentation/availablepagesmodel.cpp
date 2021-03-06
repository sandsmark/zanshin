/* This file is part of Zanshin

   Copyright 2014 Kevin Ottens <ervin@kde.org>

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


#include "availablepagesmodel.h"

#include <QIcon>
#include <QMimeData>

#include "domain/contextqueries.h"
#include "domain/contextrepository.h"
#include "domain/projectqueries.h"
#include "domain/projectrepository.h"
#include "domain/tag.h"
#include "domain/tagqueries.h"
#include "domain/tagrepository.h"
#include "domain/taskrepository.h"

#include "presentation/availablepagessortfilterproxymodel.h"
#include "presentation/contextpagemodel.h"
#include "presentation/inboxpagemodel.h"
#include "presentation/metatypes.h"
#include "presentation/projectpagemodel.h"
#include "presentation/querytreemodel.h"
#include "presentation/tagpagemodel.h"
#include "presentation/workdaypagemodel.h"

#include "utils/jobhandler.h"
#include "utils/datetime.h"

using namespace Presentation;

AvailablePagesModel::AvailablePagesModel(const Domain::ArtifactQueries::Ptr &artifactQueries,
                                         const Domain::ProjectQueries::Ptr &projectQueries,
                                         const Domain::ProjectRepository::Ptr &projectRepository,
                                         const Domain::ContextQueries::Ptr &contextQueries,
                                         const Domain::ContextRepository::Ptr &contextRepository,
                                         const Domain::TaskQueries::Ptr &taskQueries,
                                         const Domain::TaskRepository::Ptr &taskRepository,
                                         const Domain::NoteRepository::Ptr &noteRepository,
                                         const Domain::TagQueries::Ptr &tagQueries,
                                         const Domain::TagRepository::Ptr &tagRepository,
                                         QObject *parent)
    : QObject(parent),
      m_pageListModel(Q_NULLPTR),
      m_sortProxyModel(Q_NULLPTR),
      m_artifactQueries(artifactQueries),
      m_projectQueries(projectQueries),
      m_projectRepository(projectRepository),
      m_contextQueries(contextQueries),
      m_contextRepository(contextRepository),
      m_taskQueries(taskQueries),
      m_taskRepository(taskRepository),
      m_noteRepository(noteRepository),
      m_tagQueries(tagQueries),
      m_tagRepository(tagRepository)
{
}

QAbstractItemModel *AvailablePagesModel::pageListModel()
{
    if (!m_pageListModel)
        m_pageListModel = createPageListModel();

    if (!m_sortProxyModel)
        m_sortProxyModel = new AvailablePagesSortFilterProxyModel(this);
    m_sortProxyModel->setSourceModel(m_pageListModel);

    return m_sortProxyModel;
}

QObject *AvailablePagesModel::createPageForIndex(const QModelIndex &index)
{
    QObjectPtr object = index.data(QueryTreeModelBase::ObjectRole).value<QObjectPtr>();

    if (object == m_inboxObject) {
        auto inboxPageModel = new InboxPageModel(m_artifactQueries,
                                                 m_taskQueries, m_taskRepository,
                                                 m_noteRepository,
                                                 this);
        inboxPageModel->setErrorHandler(errorHandler());
        return inboxPageModel;
    } else if (object == m_workdayObject) {
        auto workdayPageModel = new WorkdayPageModel(m_taskQueries,
                                                     m_taskRepository,
                                                     m_noteRepository,
                                                     this);
        workdayPageModel->setErrorHandler(errorHandler());
        return workdayPageModel;
    } else if (auto project = object.objectCast<Domain::Project>()) {
        auto projectPageModel = new ProjectPageModel(project,
                                                     m_projectQueries,
                                                     m_taskQueries, m_taskRepository,
                                                     m_noteRepository,
                                                     this);
        projectPageModel->setErrorHandler(errorHandler());
        return projectPageModel;
    } else if (auto context = object.objectCast<Domain::Context>()) {
        auto contextPageModel = new ContextPageModel(context,
                                                     m_contextQueries,
                                                     m_taskQueries,
                                                     m_taskRepository,
                                                     m_noteRepository,
                                                     this);
        contextPageModel->setErrorHandler(errorHandler());
        return contextPageModel;
    } else if (auto tag = object.objectCast<Domain::Tag>()) {
        auto tagPageModel = new TagPageModel(tag,
                                             m_tagQueries,
                                             m_tagRepository,
                                             m_taskQueries,
                                             m_taskRepository,
                                             m_noteRepository,
                                             this);
        tagPageModel->setErrorHandler(errorHandler());
        return tagPageModel;
    }

    return Q_NULLPTR;
}

void AvailablePagesModel::addProject(const QString &name, const Domain::DataSource::Ptr &source)
{
    auto project = Domain::Project::Ptr::create();
    project->setName(name);
    const auto job = m_projectRepository->create(project, source);
    installHandler(job, tr("Cannot add project %1 in dataSource %2").arg(name).arg(source->name()));
}

void AvailablePagesModel::addContext(const QString &name)
{
    auto context = Domain::Context::Ptr::create();
    context->setName(name);
    const auto job = m_contextRepository->create(context);
    installHandler(job, tr("Cannot add context %1").arg(name));
}

void AvailablePagesModel::addTag(const QString &name)
{
    auto tag = Domain::Tag::Ptr::create();
    tag->setName(name);
    const auto job = m_tagRepository->create(tag);
    installHandler(job, tr("Cannot add tag %1").arg(name));
}

void AvailablePagesModel::removeItem(const QModelIndex &index)
{
    QObjectPtr object = index.data(QueryTreeModelBase::ObjectRole).value<QObjectPtr>();
    if (auto project = object.objectCast<Domain::Project>()) {
        const auto job = m_projectRepository->remove(project);
        installHandler(job, tr("Cannot remove project %1").arg(project->name()));
    } else if (auto context = object.objectCast<Domain::Context>()) {
        const auto job = m_contextRepository->remove(context);
        installHandler(job, tr("Cannot remove context %1").arg(context->name()));
    } else if (auto tag = object.objectCast<Domain::Tag>()) {
        const auto job = m_tagRepository->remove(tag);
        installHandler(job, tr("Cannot remove tag %1").arg(tag->name()));
    } else {
        Q_ASSERT(false);
    }
}

QAbstractItemModel *AvailablePagesModel::createPageListModel()
{
    m_inboxObject = QObjectPtr::create();
    m_inboxObject->setProperty("name", tr("Inbox"));
    m_workdayObject = QObjectPtr::create();
    m_workdayObject->setProperty("name", tr("Workday"));
    m_projectsObject = QObjectPtr::create();
    m_projectsObject->setProperty("name", tr("Projects"));
    m_contextsObject = QObjectPtr::create();
    m_contextsObject->setProperty("name", tr("Contexts"));
    m_tagsObject = QObjectPtr::create();
    m_tagsObject->setProperty("name", tr("Tags"));

    m_rootsProvider = Domain::QueryResultProvider<QObjectPtr>::Ptr::create();
    m_rootsProvider->append(m_inboxObject);
    m_rootsProvider->append(m_workdayObject);
    m_rootsProvider->append(m_projectsObject);
    m_rootsProvider->append(m_contextsObject);
    m_rootsProvider->append(m_tagsObject);

    auto query = [this](const QObjectPtr &object) -> Domain::QueryResultInterface<QObjectPtr>::Ptr {
        if (!object)
            return Domain::QueryResult<QObjectPtr>::create(m_rootsProvider);
        else if (object == m_projectsObject)
            return Domain::QueryResult<Domain::Project::Ptr, QObjectPtr>::copy(m_projectQueries->findAll());
        else if (object == m_contextsObject)
            return Domain::QueryResult<Domain::Context::Ptr, QObjectPtr>::copy(m_contextQueries->findAll());
        else if (object == m_tagsObject)
            return Domain::QueryResult<Domain::Tag::Ptr, QObjectPtr>::copy(m_tagQueries->findAll());
        else
            return Domain::QueryResult<QObjectPtr>::Ptr();
    };

    auto flags = [this](const QObjectPtr &object) {
        const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable
                                         | Qt::ItemIsEnabled
                                         | Qt::ItemIsEditable
                                         | Qt::ItemIsDropEnabled;
        const Qt::ItemFlags immutableNodeFlags = Qt::ItemIsSelectable
                                               | Qt::ItemIsEnabled
                                               | Qt::ItemIsDropEnabled;
        const Qt::ItemFlags structureNodeFlags = Qt::NoItemFlags;

        return object.objectCast<Domain::Project>() ? defaultFlags
             : object.objectCast<Domain::Context>() ? defaultFlags
             : object.objectCast<Domain::Tag>() ? defaultFlags
             : object == m_inboxObject ? immutableNodeFlags
             : object == m_workdayObject ? immutableNodeFlags
             : structureNodeFlags;
    };

    auto data = [this](const QObjectPtr &object, int role) -> QVariant {
        if (role != Qt::DisplayRole
         && role != Qt::EditRole
         && role != Qt::DecorationRole
         && role != QueryTreeModelBase::IconNameRole) {
            return QVariant();
        }

        if (role == Qt::EditRole
         && (object == m_inboxObject
          || object == m_workdayObject
          || object == m_projectsObject
          || object == m_contextsObject
          || object == m_tagsObject)) {
            return QVariant();
        }

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return object->property("name").toString();
        } else if (role == Qt::DecorationRole || role == QueryTreeModelBase::IconNameRole) {
            const QString iconName = object == m_inboxObject ? "mail-folder-inbox"
                                   : (object == m_workdayObject)  ? "go-jump-today"
                                   : (object == m_projectsObject) ? "folder"
                                   : (object == m_contextsObject) ? "folder"
                                   : (object == m_tagsObject)     ? "folder"
                                   : "view-pim-tasks";

            if (role == Qt::DecorationRole)
                return QVariant::fromValue(QIcon::fromTheme(iconName));
            else
                return iconName;
        } else {
            return QVariant();
        }
    };

    auto setData = [this](const QObjectPtr &object, const QVariant &value, int role) {
        if (role != Qt::EditRole) {
            return false;
        }

        if (object == m_inboxObject
         || object == m_workdayObject
         || object == m_projectsObject
         || object == m_contextsObject
         || object == m_tagsObject) {
            return false;
        }

        if (auto project = object.objectCast<Domain::Project>()) {
            const auto currentName = project->name();
            project->setName(value.toString());
            const auto job = m_projectRepository->update(project);
            installHandler(job, tr("Cannot modify project %1").arg(currentName));
        } else if (auto context = object.objectCast<Domain::Context>()) {
            const auto currentName = context->name();
            context->setName(value.toString());
            const auto job = m_contextRepository->update(context);
            installHandler(job, tr("Cannot modify context %1").arg(currentName));
        } else if (object.objectCast<Domain::Tag>()) {
            return false; // Tag renaming is NOT allowed
        } else {
            Q_ASSERT(false);
        }

        return true;
    };

    auto drop = [this](const QMimeData *mimeData, Qt::DropAction, const QObjectPtr &object) {
        if (!mimeData->hasFormat("application/x-zanshin-object"))
            return false;

        auto droppedArtifacts = mimeData->property("objects").value<Domain::Artifact::List>();
        if (droppedArtifacts.isEmpty())
            return false;

        if (auto project = object.objectCast<Domain::Project>()) {
            foreach (const auto &droppedArtifact, droppedArtifacts) {
                const auto job = m_projectRepository->associate(project, droppedArtifact);
                installHandler(job, tr("Cannot add %1 to project %2").arg(droppedArtifact->title()).arg(project->name()));
            }
            return true;
        } else if (auto context = object.objectCast<Domain::Context>()) {
            if (std::any_of(droppedArtifacts.begin(), droppedArtifacts.end(),
                            [](const Domain::Artifact::Ptr &droppedArtifact) {
                                return !droppedArtifact.objectCast<Domain::Task>();
                            })) {
                return false;
            }
            foreach (const auto &droppedArtifact, droppedArtifacts) {
                auto task = droppedArtifact.staticCast<Domain::Task>();
                const auto job = m_contextRepository->associate(context, task);
                installHandler(job, tr("Cannot add %1 to context %2").arg(task->title()).arg(context->name()));
            }
            return true;
        } else if (auto tag = object.objectCast<Domain::Tag>()) {
            foreach (const auto &droppedArtifact, droppedArtifacts) {
                const auto job = m_tagRepository->associate(tag, droppedArtifact);
                installHandler(job, tr("Cannot tag %1 with %2").arg(droppedArtifact->title()).arg(tag->name()));
            }
            return true;
        } else if (object == m_inboxObject) {
            foreach (const auto &droppedArtifact, droppedArtifacts) {
                const auto job = m_projectRepository->dissociate(droppedArtifact);
                installHandler(job, tr("Cannot move %1 to Inbox").arg(droppedArtifact->title()));

                if (auto task = droppedArtifact.objectCast<Domain::Task>()) {
                    Utils::JobHandler::install(job, [this, task] {
                        const auto dissociateJob = m_taskRepository->dissociateAll(task);
                        installHandler(dissociateJob, tr("Cannot move task %1 to Inbox").arg(task->title()));
                    });
                }
            }
            return true;
        } else if (object == m_workdayObject) {
            foreach (const auto &droppedArtifact, droppedArtifacts) {

                if (auto task = droppedArtifact.objectCast<Domain::Task>()) {

                    task->setStartDate(Utils::DateTime::currentDateTime());
                    const auto job = m_taskRepository->update(task);

                    installHandler(job, tr("Cannot update task %1 to Workday").arg(task->title()));
                }
            }
            return true;
        }

        return false;
    };

    auto drag = [](const QObjectPtrList &) -> QMimeData* {
        return Q_NULLPTR;
    };

    return new QueryTreeModel<QObjectPtr>(query, flags, data, setData, drop, drag, this);
}
