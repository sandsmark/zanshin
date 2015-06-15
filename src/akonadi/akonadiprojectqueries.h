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

#ifndef AKONADI_PROJECTQUERIES_H
#define AKONADI_PROJECTQUERIES_H

#include "domain/projectqueries.h"

#include <AkonadiCore/Item>

#include "akonadi/akonadimonitorinterface.h"
#include "akonadi/akonadiserializerinterface.h"
#include "akonadi/akonadistorageinterface.h"

#include "domain/livequery.h"

namespace Akonadi {

class ProjectQueries : public QObject, public Domain::ProjectQueries
{
    Q_OBJECT
public:
    typedef QSharedPointer<ProjectQueries> Ptr;

    typedef Domain::LiveQuery<Akonadi::Item, Domain::Project::Ptr> ProjectQuery;
    typedef Domain::QueryResultProvider<Domain::Project::Ptr> ProjectProvider;
    typedef Domain::QueryResult<Domain::Project::Ptr> ProjectResult;

    typedef Domain::LiveQuery<Akonadi::Item, Domain::Artifact::Ptr> ArtifactQuery;
    typedef Domain::QueryResultProvider<Domain::Artifact::Ptr> ArtifactProvider;
    typedef Domain::QueryResult<Domain::Artifact::Ptr> ArtifactResult;

    ProjectQueries(const StorageInterface::Ptr &storage,
                   const SerializerInterface::Ptr &serializer,
                   const MonitorInterface::Ptr &monitor);

    ProjectResult::Ptr findAll() const Q_DECL_OVERRIDE;
    ArtifactResult::Ptr findTopLevelArtifacts(Domain::Project::Ptr project) const Q_DECL_OVERRIDE;

private slots:
    void onItemAdded(const Akonadi::Item &item);
    void onItemRemoved(const Akonadi::Item &item);
    void onItemChanged(const Akonadi::Item &item);
    void onCollectionSelectionChanged();

private:
    ProjectQuery::Ptr createProjectQuery();
    ArtifactQuery::Ptr createArtifactQuery();

    StorageInterface::Ptr m_storage;
    SerializerInterface::Ptr m_serializer;
    MonitorInterface::Ptr m_monitor;

    ProjectQuery::Ptr m_findAll;
    ProjectQuery::List m_projectQueries;

    QHash<Akonadi::Entity::Id, ArtifactQuery::Ptr> m_findTopLevel;
    ArtifactQuery::List m_artifactQueries;
};

}

#endif // AKONADI_PROJECTQUERIES_H
