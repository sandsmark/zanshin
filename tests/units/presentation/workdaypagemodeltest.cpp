/* This file is part of Zanshin

   Copyright 2015 Theo Vaucher <theo.vaucher@gmail.com>

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

#include <QtTest>

#include <testlib/fakejob.h>

#include "utils/datetime.h"
#include "utils/mockobject.h"

#include "domain/noterepository.h"
#include "domain/taskqueries.h"
#include "domain/taskrepository.h"
#include "presentation/workdaypagemodel.h"

using namespace mockitopp;
using namespace mockitopp::matcher;

class WorkdayPageModelTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldListWorkdayInCentralListModel()
    {
        // GIVEN

        const auto today = Utils::DateTime::currentDateTime();

        // Three tasks
        auto task1 = Domain::Task::Ptr::create();
        task1->setTitle("task1");
        task1->setStartDate(today.addDays(-10));
        task1->setDueDate(today);
        auto task2 = Domain::Task::Ptr::create();
        task2->setTitle("task2");
        task2->setStartDate(today);
        task2->setDueDate(today.addDays(10));
        auto task3 = Domain::Task::Ptr::create();
        task3->setTitle("task3");
        task3->setStartDate(today.addYears(-4));
        task3->setDueDate(today.addYears(-3));
        auto taskProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto taskResult = Domain::QueryResult<Domain::Task::Ptr>::create(taskProvider);
        taskProvider->append(task1);
        taskProvider->append(task2);
        taskProvider->append(task3);

        // Two tasks under the task1
        auto childTask11 = Domain::Task::Ptr::create();
        childTask11->setTitle("childTask11");
        auto childTask12 = Domain::Task::Ptr::create();
        childTask12->setTitle("childTask12");
        childTask12->setStartDate(today);
        childTask12->setDueDate(today);
        auto childTaskProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto childTaskResult = Domain::QueryResult<Domain::Task::Ptr>::create(childTaskProvider);
        taskProvider->append(childTask12);
        childTaskProvider->append(childTask11);
        childTaskProvider->append(childTask12);

        Utils::MockObject<Domain::TaskQueries> taskQueriesMock;
        taskQueriesMock(&Domain::TaskQueries::findWorkdayTopLevel).when().thenReturn(taskResult);
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(task1).thenReturn(childTaskResult);
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(task2).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(task3).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(childTask11).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(childTask12).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());

        Utils::MockObject<Domain::TaskRepository> taskRepositoryMock;
        Utils::MockObject<Domain::NoteRepository> noteRepositoryMock;

        Presentation::WorkdayPageModel workday(taskQueriesMock.getInstance(),
                                               taskRepositoryMock.getInstance(),
                                               noteRepositoryMock.getInstance());

        // WHEN
        QAbstractItemModel *model = workday.centralListModel();

        // THEN
        const QModelIndex task1Index = model->index(0, 0);
        const QModelIndex task2Index = model->index(1, 0);
        const QModelIndex task3Index = model->index(2, 0);
        const QModelIndex taskChildTask12Index = model->index(3, 0);

        const QModelIndex childTask11Index = model->index(0, 0, task1Index);
        const QModelIndex childTask12Index = model->index(1, 0, task1Index);

        QCOMPARE(model->rowCount(), 4);
        QCOMPARE(model->rowCount(task1Index), 2);
        QCOMPARE(model->rowCount(task2Index), 0);
        QCOMPARE(model->rowCount(task3Index), 0);
        QCOMPARE(model->rowCount(taskChildTask12Index), 0);

        QVERIFY(childTask11Index.isValid());
        QVERIFY(childTask12Index.isValid());
        QCOMPARE(model->rowCount(childTask11Index), 0);
        QCOMPARE(model->rowCount(childTask12Index), 0);

        const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable
                                         | Qt::ItemIsEnabled
                                         | Qt::ItemIsEditable
                                         | Qt::ItemIsDragEnabled;
        QCOMPARE(model->flags(task1Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(childTask11Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(childTask12Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(task2Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(task3Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(taskChildTask12Index), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);

        QCOMPARE(model->data(task1Index).toString(), task1->title());
        QCOMPARE(model->data(childTask11Index).toString(), childTask11->title());
        QCOMPARE(model->data(childTask12Index).toString(), childTask12->title());
        QCOMPARE(model->data(task2Index).toString(), task2->title());
        QCOMPARE(model->data(task3Index).toString(), task3->title());
        QCOMPARE(model->data(taskChildTask12Index).toString(), childTask12->title());

        QCOMPARE(model->data(task1Index, Qt::EditRole).toString(), task1->title());
        QCOMPARE(model->data(childTask11Index, Qt::EditRole).toString(), childTask11->title());
        QCOMPARE(model->data(childTask12Index, Qt::EditRole).toString(), childTask12->title());
        QCOMPARE(model->data(task2Index, Qt::EditRole).toString(), task2->title());
        QCOMPARE(model->data(task3Index, Qt::EditRole).toString(), task3->title());
        QCOMPARE(model->data(taskChildTask12Index, Qt::EditRole).toString(), childTask12->title());

        QVERIFY(model->data(task1Index, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(childTask11Index, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(childTask12Index, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(task2Index, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(task3Index, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(taskChildTask12Index, Qt::CheckStateRole).isValid());

        QCOMPARE(model->data(task1Index, Qt::CheckStateRole).toBool(), task1->isDone());
        QCOMPARE(model->data(childTask11Index, Qt::CheckStateRole).toBool(), childTask11->isDone());
        QCOMPARE(model->data(childTask12Index, Qt::CheckStateRole).toBool(), childTask12->isDone());
        QCOMPARE(model->data(task2Index, Qt::CheckStateRole).toBool(), task2->isDone());
        QCOMPARE(model->data(task3Index, Qt::CheckStateRole).toBool(), task3->isDone());
        QCOMPARE(model->data(taskChildTask12Index, Qt::CheckStateRole).toBool(), childTask12->isDone());

        // WHEN
        taskRepositoryMock(&Domain::TaskRepository::update).when(task1).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::update).when(childTask11).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::update).when(childTask12).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::update).when(task2).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::update).when(task3).thenReturn(new FakeJob(this));

        QVERIFY(model->setData(task1Index, "newTask1"));
        QVERIFY(model->setData(childTask11Index, "newChildTask11"));
        QVERIFY(model->setData(task2Index, "newTask2"));
        QVERIFY(model->setData(task3Index, "newTask3"));
        QVERIFY(model->setData(taskChildTask12Index, "newChildTask12"));

        QVERIFY(model->setData(task1Index, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(model->setData(childTask11Index, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(model->setData(task2Index, Qt::Checked, Qt::CheckStateRole));
        QVERIFY(model->setData(task3Index, Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(model->setData(taskChildTask12Index, Qt::Checked, Qt::CheckStateRole));

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(task1).exactly(2));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(childTask11).exactly(2));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(childTask12).exactly(2));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(task2).exactly(2));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(task3).exactly(2));

        QCOMPARE(task1->title(), QString("newTask1"));
        QCOMPARE(childTask11->title(), QString("newChildTask11"));
        QCOMPARE(childTask12->title(), QString("newChildTask12"));
        QCOMPARE(task2->title(), QString("newTask2"));
        QCOMPARE(task3->title(), QString("newTask3"));

        QCOMPARE(task1->isDone(), false);
        QCOMPARE(childTask11->isDone(), false);
        QCOMPARE(childTask12->isDone(), true);
        QCOMPARE(task2->isDone(), true);
        QCOMPARE(task3->isDone(), false);
    }

    void shouldAddTasks()
    {
        // GIVEN

        // ... in fact we won't list any model
        Utils::MockObject<Domain::TaskQueries> taskQueriesMock;

        // Nor create notes...
        Utils::MockObject<Domain::NoteRepository> noteRepositoryMock;

        // We'll gladly create a task though
        Utils::MockObject<Domain::TaskRepository> taskRepositoryMock;
        taskRepositoryMock(&Domain::TaskRepository::create).when(any<Domain::Task::Ptr>()).thenReturn(new FakeJob(this));

        Presentation::WorkdayPageModel workday(taskQueriesMock.getInstance(),
                                               taskRepositoryMock.getInstance(),
                                               noteRepositoryMock.getInstance());

        // WHEN
        auto title = QString("New task");
        auto today = Utils::DateTime::currentDateTime();
        auto task = workday.addTask(title);

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::create).when(any<Domain::Task::Ptr>()).exactly(1));

        QCOMPARE(task->title(), title);
        QCOMPARE(task->startDate(), today);
    }

    void shouldDeleteItems()
    {
        // GIVEN

        // Two tasks
        auto task1 = Domain::Task::Ptr::create();
        auto task2 = Domain::Task::Ptr::create();
        auto taskProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto taskResult = Domain::QueryResult<Domain::Task::Ptr>::create(taskProvider);
        taskProvider->append(task1);
        taskProvider->append(task2);

        Utils::MockObject<Domain::TaskQueries> taskQueriesMock;
        taskQueriesMock(&Domain::TaskQueries::findWorkdayTopLevel).when().thenReturn(taskResult);
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(task1).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(task2).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());

        Utils::MockObject<Domain::NoteRepository> noteRepositoryMock;

        Utils::MockObject<Domain::TaskRepository> taskRepositoryMock;
        taskRepositoryMock(&Domain::TaskRepository::remove).when(task2).thenReturn(new FakeJob(this));

        Presentation::WorkdayPageModel workday(taskQueriesMock.getInstance(),
                                               taskRepositoryMock.getInstance(),
                                               noteRepositoryMock.getInstance());

        // WHEN
        const QModelIndex index = workday.centralListModel()->index(1, 0);
        workday.removeItem(index);

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::remove).when(task2).exactly(1));
    }
};

QTEST_MAIN(WorkdayPageModelTest)

#include "workdaypagemodeltest.moc"
