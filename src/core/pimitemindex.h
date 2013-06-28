/*
 * Copyright (C) 2013 Kevin Ottens <ervin@kde.org>
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

#ifndef PIMITEMINDEX_H
#define PIMITEMINDEX_H

#include <Akonadi/Item>
#include <Akonadi/Collection>
#include "globaldefs.h"

class PimItemIndex
{
public:
    enum ItemType {
        NoType,

        Inbox,
        FolderRoot,

        Project,
        Context,
        Topic,

        Collection,
        Note,
        Event,
        Todo,
        Journal
    };

    PimItemIndex();
    PimItemIndex(ItemType t);

    ItemType type;
    Akonadi::Item item;
    Id relationId;
    Akonadi::Collection collection;
    QString uid;
};

Q_DECLARE_METATYPE(PimItemIndex)

#endif // PIMITEMINDEX_H