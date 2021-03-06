/* This file is part of Zanshin

   Copyright 2014 Mario Bensi <mbensi@ipsquad.net>

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

#include "monitorspy.h"
#include <QTest>

MonitorSpy::MonitorSpy(Akonadi::MonitorInterface* monitor, QObject *parent)
    : QObject(parent),
      m_monitor(monitor),
      m_timer(new QTimer(this)),
      m_isFinished(false)
{
    connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(collectionSelectionChanged(Akonadi::Collection)), this, SLOT(restartTimer()));

    connect(m_monitor, SIGNAL(itemAdded(Akonadi::Item)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(itemMoved(Akonadi::Item)), this, SLOT(restartTimer()));

    connect(m_monitor, SIGNAL(tagAdded(Akonadi::Tag)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(tagRemoved(Akonadi::Tag)), this, SLOT(restartTimer()));
    connect(m_monitor, SIGNAL(tagChanged(Akonadi::Tag)), this, SLOT(restartTimer()));

    connect(m_timer, SIGNAL(timeout()), this, SLOT(onDelayExpired()));
}

void MonitorSpy::waitForStableState()
{
    m_isFinished = false;
    m_timer->start(1000);
    while (!m_isFinished)
        QTest::qWait(20);
}

void MonitorSpy::restartTimer()
{
    m_timer->start(1000);
}

void MonitorSpy::onDelayExpired()
{
    m_isFinished = true;
}
