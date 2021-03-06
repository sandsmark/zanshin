/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>
   Copyright 2008,2009 Mario Bensi <nef@ipsquad.net>

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

#include "maincomponent.h"

#include <KDE/Akonadi/AgentManager>
#include <KDE/Akonadi/AgentInstance>
#include <KDE/Akonadi/AgentType>

#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KConfigGroup>
#include <KDE/KIcon>
#include <KDE/KLocale>
#include <KDE/KXMLGUIClient>

#include <QtCore/QTimer>
#include <QtGui/QHeaderView>

#include "configdialog.h"
#include "globaldefs.h"
#include "gui/sidebar/sidebar.h"
#include "gui/itemeditor/itemeditor.h"
#include "gui/itemlist/actionlisteditor.h"
#include "gui/itemlist/itemselectorproxy.h"
#include "core/modelstack.h"
#include "core/akonadidatastore.h"
#include "utils/dependencymanager.h"

MainComponent::MainComponent(ModelStack *models, QWidget *parent, KXMLGUIClient *client)
    : QObject(parent),
        m_itemViewer(0)
{
    Utils::DependencyManager::globalInstance().add<DataStoreInterface, AkonadiDataStore>();

    KActionCollection *ac = client->actionCollection();

    m_sidebar = new SideBar(models, ac, parent);
    m_itemViewer = new ItemEditor(parent, client);
    m_editor = new ActionListEditor(models,
                                    ac, parent, client, m_itemViewer);
    setupActions(ac);

    ac->action("project_mode")->trigger();
}

SideBar *MainComponent::sideBar() const
{
    return m_sidebar;
}

ActionListEditor *MainComponent::editor() const
{
    return m_editor;
}

void MainComponent::setupActions(KActionCollection *ac)
{
    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    KAction *action = ac->addAction("project_mode", this, SLOT(onModeSwitch()));
    action->setText(i18n("Project View"));
    action->setIcon(KIcon("view-pim-tasks"));
    action->setShortcut(Qt::CTRL | Qt::Key_P);
    action->setCheckable(true);
    action->setData(Zanshin::ProjectMode);
    modeGroup->addAction(action);

    action = ac->addAction("contexts_mode", this, SLOT(onModeSwitch()));
    action->setText(i18n("Context View"));
    action->setIcon(KIcon("view-pim-notes"));
    action->setShortcut(Qt::CTRL | Qt::Key_O);
    action->setCheckable(true);
    action->setData(Zanshin::ContextsMode);
    modeGroup->addAction(action);

    action = ac->addAction("knowledge_mode", this, SLOT(onModeSwitch()));
    action->setText(i18n("Knowledge View"));
    action->setIcon(KIcon("view-pim-notes"));
    action->setShortcut(Qt::CTRL | Qt::Key_K);
    action->setCheckable(true);
    action->setData(Zanshin::KnowledgeMode);
    modeGroup->addAction(action);

    action = ac->addAction("synchronize_all", this, SLOT(onSynchronizeAll()));
    action->setText(i18n("Synchronize All"));
    action->setIcon(KIcon("view-refresh"));
    action->setShortcut(Qt::CTRL | Qt::Key_L);
}

void MainComponent::saveColumnsState(KConfigGroup &cg) const
{
    m_editor->saveColumnsState(cg);
}

void MainComponent::restoreColumnsState(const KConfigGroup &cg)
{
    m_editor->restoreColumnsState(cg);
}

void MainComponent::onModeSwitch()
{
    KAction *action = static_cast<KAction*>(sender());
    m_editor->setMode((Zanshin::ApplicationMode)action->data().toInt());
    m_sidebar->setMode((Zanshin::ApplicationMode)action->data().toInt());
}

void MainComponent::onSynchronizeAll()
{
    Akonadi::AgentInstance::List agents = Akonadi::AgentManager::self()->instances();
    while (!agents.isEmpty()) {
        Akonadi::AgentInstance agent = agents.takeFirst();

        if (agent.type().mimeTypes().contains(PimItem::mimeType(PimItem::Todo)) ||
            agent.type().mimeTypes().contains(PimItem::mimeType(PimItem::Note))) {
            agent.synchronize();
        }
    }
}

void MainComponent::showConfigDialog()
{
    ConfigDialog dialog(static_cast<QWidget*>(parent()));
    dialog.exec();
}

ItemEditor* MainComponent::itemViewer() const
{
    return m_itemViewer;
}
