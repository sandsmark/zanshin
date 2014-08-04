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


#include "editorview.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>

#include "pimlibs/kdateedit.h"

using namespace Widgets;

EditorView::EditorView(QWidget *parent)
    : QWidget(parent),
      m_model(0),
      m_textEdit(new QPlainTextEdit(this)),
      m_taskGroup(new QWidget(this)),
      m_startDateEdit(new KPIM::KDateEdit(m_taskGroup)),
      m_dueDateEdit(new KPIM::KDateEdit(m_taskGroup)),
      m_doneButton(new QCheckBox(tr("Done"), m_taskGroup))
{
    m_textEdit->setObjectName("textEdit");
    m_startDateEdit->setObjectName("startDateEdit");
    m_dueDateEdit->setObjectName("dueDateEdit");
    m_doneButton->setObjectName("doneButton");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_textEdit);
    layout->addWidget(m_taskGroup);
    setLayout(layout);

    QVBoxLayout *vbox = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Start date"), m_taskGroup));
    hbox->addWidget(m_startDateEdit);
    hbox->addWidget(new QLabel(tr("Due date"), m_taskGroup));
    hbox->addWidget(m_dueDateEdit);
    vbox->addLayout(hbox);
    vbox->addWidget(m_doneButton);
    m_taskGroup->setLayout(vbox);

    m_taskGroup->setVisible(false);

    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()));
    connect(m_startDateEdit, SIGNAL(dateEntered(QDate)), this, SLOT(onStartEditEntered(QDate)));
    connect(m_dueDateEdit, SIGNAL(dateEntered(QDate)), this, SLOT(onDueEditEntered(QDate)));
    connect(m_doneButton, SIGNAL(toggled(bool)), this, SLOT(onDoneButtonChanged(bool)));
}

QObject *EditorView::model() const
{
    return m_model;
}

void EditorView::setModel(QObject *model)
{
    if (model == m_model)
        return;

    if (m_model) {
        disconnect(m_model, 0, this, 0);
        disconnect(this, 0, m_model, 0);
    }

    m_model = model;

    onTextOrTitleChanged();
    onHasTaskPropertiesChanged();
    onStartDateChanged();
    onDueDateChanged();
    onDoneChanged();

    connect(m_model, SIGNAL(hasTaskPropertiesChanged(bool)),
            this, SLOT(onHasTaskPropertiesChanged()));
    connect(m_model, SIGNAL(titleChanged(QString)), this, SLOT(onTextOrTitleChanged()));
    connect(m_model, SIGNAL(textChanged(QString)), this, SLOT(onTextOrTitleChanged()));
    connect(m_model, SIGNAL(startDateChanged(QDateTime)), this, SLOT(onStartDateChanged()));
    connect(m_model, SIGNAL(dueDateChanged(QDateTime)), this, SLOT(onDueDateChanged()));
    connect(m_model, SIGNAL(doneChanged(bool)), this, SLOT(onDoneChanged()));

    connect(this, SIGNAL(titleChanged(QString)), m_model, SLOT(setTitle(QString)));
    connect(this, SIGNAL(textChanged(QString)), m_model, SLOT(setText(QString)));
    connect(this, SIGNAL(startDateChanged(QDateTime)), m_model, SLOT(setStartDate(QDateTime)));
    connect(this, SIGNAL(dueDateChanged(QDateTime)), m_model, SLOT(setDueDate(QDateTime)));
    connect(this, SIGNAL(doneChanged(bool)), m_model, SLOT(setDone(bool)));
}

void EditorView::onHasTaskPropertiesChanged()
{
    m_taskGroup->setVisible(m_model->property("hasTaskProperties").toBool());
}

void EditorView::onTextOrTitleChanged()
{
    const QString text = m_model->property("title").toString()
                       + "\n"
                       + m_model->property("text").toString();

    if (text != m_textEdit->toPlainText())
        m_textEdit->setPlainText(text);
}

void EditorView::onStartDateChanged()
{
    m_startDateEdit->setDate(m_model->property("startDate").toDateTime().date());
}

void EditorView::onDueDateChanged()
{
    m_dueDateEdit->setDate(m_model->property("dueDate").toDateTime().date());
}

void EditorView::onDoneChanged()
{
    m_doneButton->setChecked(m_model->property("done").toBool());
}

void EditorView::onTextEditChanged()
{
    const QString plainText = m_textEdit->toPlainText();
    const int index = plainText.indexOf('\n');
    const QString title = plainText.left(index);
    const QString text = plainText.mid(index + 1);
    emit titleChanged(title);
    emit textChanged(text);
}

void EditorView::onStartEditEntered(const QDate &start)
{
    emit startDateChanged(QDateTime(start));
}

void EditorView::onDueEditEntered(const QDate &due)
{
    emit dueDateChanged(QDateTime(due));
}

void EditorView::onDoneButtonChanged(bool checked)
{
    emit doneChanged(checked);
}