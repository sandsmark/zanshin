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


#ifndef PIMITEMRELATIONS_H
#define PIMITEMRELATIONS_H
#include <QByteArray>
#include <QString>
#include <QList>

struct PimItemTreeNode {
    PimItemTreeNode(const QByteArray &uid, const QString &name, const QList<PimItemTreeNode> &parentNodes);
    PimItemTreeNode(const QByteArray &uid, const QString &name = QString());
    QByteArray uid;
    QString name;
    QList<PimItemTreeNode> parentNodes;
    bool knowsParents;
};

struct PimItemRelation
{
  enum Type {
    Invalid,
    Project,
    Context,
    Topic
  };
  
  PimItemRelation(Type type, const QList<PimItemTreeNode> &parentNodes);
  PimItemRelation();
  
  //     QDateTime timestamp; //for merging
  Type type;
  QList<PimItemTreeNode> parentNodes;
};

PimItemRelation relationFromXML(const QByteArray &xml);
QString relationToXML(const PimItemRelation &rel);
PimItemRelation removeDuplicates(const PimItemRelation &);

#endif // PIMITEMRELATIONS_H
