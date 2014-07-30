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


#include "inboxpagemodel.h"

#include "domain/artifactqueries.h"
#include "domain/noterepository.h"
#include "domain/taskqueries.h"
#include "domain/taskrepository.h"

#include "presentation/querytreemodel.h"

#include "utils/dependencymanager.h"

using namespace Presentation;

InboxPageModel::InboxPageModel(QObject *parent)
    : QObject(parent),
      m_centralListModel(0),
      m_artifactQueries(Utils::DependencyManager::globalInstance().create<Domain::ArtifactQueries>()),
      m_sourceQueries(Utils::DependencyManager::globalInstance().create<Domain::DataSourceQueries>()),
      m_taskQueries(Utils::DependencyManager::globalInstance().create<Domain::TaskQueries>()),
      m_taskRepository(Utils::DependencyManager::globalInstance().create<Domain::TaskRepository>()),
      m_taskSources(m_sourceQueries->findTasks()),
      m_noteRepository(Utils::DependencyManager::globalInstance().create<Domain::NoteRepository>()),
      m_noteSources(m_sourceQueries->findNotes()),
      m_ownInterface(true)
{
    qRegisterMetaType<Domain::DataSource::Ptr>();
    qRegisterMetaType<QAbstractItemModel*>();
}

InboxPageModel::InboxPageModel(Domain::ArtifactQueries *artifactQueries,
                       Domain::DataSourceQueries *sourceQueries,
                       Domain::TaskQueries *taskQueries,
                       Domain::TaskRepository *taskRepository,
                       Domain::NoteRepository *noteRepository,
                       QObject *parent)
    : QObject(parent),
      m_centralListModel(0),
      m_artifactQueries(artifactQueries),
      m_sourceQueries(sourceQueries),
      m_taskQueries(taskQueries),
      m_taskRepository(taskRepository),
      m_taskSources(m_sourceQueries->findTasks()),
      m_noteRepository(noteRepository),
      m_noteSources(m_sourceQueries->findNotes()),
      m_ownInterface(false)
{
    qRegisterMetaType<Domain::DataSource::Ptr>();
    qRegisterMetaType<QAbstractItemModel*>();
}

InboxPageModel::~InboxPageModel()
{
}

QAbstractItemModel *InboxPageModel::centralListModel()
{
    if (!m_centralListModel)
        m_centralListModel = createCentralListModel();
    return m_centralListModel;
}

Domain::DataSource::Ptr InboxPageModel::defaultNoteDataSource() const
{
    QList<Domain::DataSource::Ptr> sources = m_noteSources->data();

    if (sources.isEmpty())
        return Domain::DataSource::Ptr();

    auto source = std::find_if(sources.begin(), sources.end(),
                               [this] (const Domain::DataSource::Ptr &source) {
                                   return m_noteRepository->isDefaultSource(source);
                               });

    if (source != sources.end())
        return *source;
    else
        return sources.first();
}

Domain::DataSource::Ptr InboxPageModel::defaultTaskDataSource() const
{
    QList<Domain::DataSource::Ptr> sources = m_taskSources->data();

    if (sources.isEmpty())
        return Domain::DataSource::Ptr();

    auto source = std::find_if(sources.begin(), sources.end(),
                               [this] (const Domain::DataSource::Ptr &source) {
                                   return m_taskRepository->isDefaultSource(source);
                               });

    if (source != sources.end())
        return *source;
    else
        return sources.first();
}

void InboxPageModel::addTask(const QString &title)
{
    auto task = Domain::Task::Ptr::create();
    task->setTitle(title);
    m_taskRepository->save(task);
}

void InboxPageModel::removeItem(const QModelIndex &index)
{
    QVariant data = index.data(QueryTreeModel<Domain::Artifact::Ptr>::ObjectRole);
    auto artifact = data.value<Domain::Artifact::Ptr>();
    auto task = artifact.objectCast<Domain::Task>();
    m_taskRepository->remove(task);
}

void InboxPageModel::setDefaultNoteDataSource(Domain::DataSource::Ptr source)
{
    m_noteRepository->setDefaultSource(source);
}

void InboxPageModel::setDefaultTaskDataSource(Domain::DataSource::Ptr source)
{
    m_taskRepository->setDefaultSource(source);
}

QAbstractItemModel *InboxPageModel::createCentralListModel()
{
    auto query = [this](const Domain::Artifact::Ptr &artifact) -> Domain::QueryResultInterface<Domain::Artifact::Ptr>::Ptr {
        if (!artifact)
            return m_artifactQueries->findInboxTopLevel();
        else if (auto task = artifact.dynamicCast<Domain::Task>())
            return Domain::QueryResult<Domain::Task::Ptr, Domain::Artifact::Ptr>::copy(m_taskQueries->findChildren(task));
        else
            return Domain::QueryResult<Domain::Artifact::Ptr>::Ptr();
    };

    auto flags = [](const Domain::Artifact::Ptr &artifact) {
        const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable
                                         | Qt::ItemIsEnabled
                                         | Qt::ItemIsEditable;

        return artifact.dynamicCast<Domain::Task>() ? (defaultFlags | Qt::ItemIsUserCheckable) : defaultFlags;
    };

    auto data = [](const Domain::Artifact::Ptr &artifact, int role) -> QVariant {
        if (role != Qt::DisplayRole
         && role != Qt::EditRole
         && role != Qt::CheckStateRole) {
            return QVariant();
        }

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return artifact->title();
        } else if (auto task = artifact.dynamicCast<Domain::Task>()) {
            return task->isDone() ? Qt::Checked : Qt::Unchecked;
        } else {
            return QVariant();
        }
    };

    auto setData = [this](const Domain::Artifact::Ptr &artifact, const QVariant &value, int role) {
        if (role != Qt::EditRole && role != Qt::CheckStateRole) {
            return false;
        }

        if (auto task = artifact.dynamicCast<Domain::Task>()) {
            if (role == Qt::EditRole)
                task->setTitle(value.toString());
            else
                task->setDone(value.toInt() == Qt::Checked);

            m_taskRepository->save(task);
            return true;

        } else if (auto note = artifact.dynamicCast<Domain::Note>()) {
            if (role != Qt::EditRole)
                return false;

            note->setTitle(value.toString());
            m_noteRepository->save(note);
            return true;

        }

        return false;
    };

    return new QueryTreeModel<Domain::Artifact::Ptr>(query, flags, data, setData, this);
}
