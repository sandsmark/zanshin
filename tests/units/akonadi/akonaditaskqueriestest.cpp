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

#include <QtTest>

#include "utils/datetime.h"
#include "utils/mockobject.h"

#include "testlib/akonadifakejobs.h"
#include "testlib/akonadifakemonitor.h"

#include "akonadi/akonaditaskqueries.h"
#include "akonadi/akonadiserializerinterface.h"
#include "akonadi/akonadistorageinterface.h"

using namespace mockitopp;

Q_DECLARE_METATYPE(Testlib::AkonadiFakeItemFetchJob*)
Q_DECLARE_METATYPE(Testlib::AkonadiFakeCollectionFetchJob*)

class AkonadiTaskQueriesTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldLookInAllReportedForAllTasks()
    {
        // GIVEN

        // Two top level collections
        Akonadi::Collection col1(42);
        col1.setParentCollection(Akonadi::Collection::root());
        Akonadi::Collection col2(43);
        col2.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col1 << col2);

        // One task in the first collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col1);
        Domain::Task::Ptr task1(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);

        // Two tasks in the second collection
        Akonadi::Item item2(43);
        item2.setParentCollection(col2);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col2);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();
        result->data();
        result = queries->findAll(); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col1).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 3);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
        QCOMPARE(result->data().at(2), task3);
    }

    void shouldIgnoreItemsWhichAreNotTasks()
    {
        // GIVEN

        // Two top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Two tasks in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        // One of them is not a task
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2;
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(false);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(0));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().at(0), task1);
    }

    void shouldReactToItemAddsForTasksOnly()
    {
        // GIVEN

        // Empty collection fetch
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();
        QTest::qWait(150);
        QVERIFY(result->data().isEmpty());

        // WHEN
        Akonadi::Item item1(42);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        Domain::Task::Ptr task2;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(false);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        monitor->addItem(item1);
        monitor->addItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(0));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().first(), task1);
    }

    void shouldReactToItemRemovesForAllTasks()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 3);

        // WHEN
        monitor->removeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task3);
    }

    void shouldReactToItemChangesForAllTasks()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);
        serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).thenReturn();

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn(QString());

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();
        // Even though the pointer didn't change it's convenient to user if we call
        // the replace handlers
        bool replaceHandlerCalled = false;
        result->addPostReplaceHandler([&replaceHandlerCalled](const Domain::Task::Ptr &, int) {
                                          replaceHandlerCalled = true;
                                      });
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 3);

        // WHEN
        monitor->changeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).exactly(1));

        QCOMPARE(result->data().size(), 3);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
        QCOMPARE(result->data().at(2), task3);
        QVERIFY(replaceHandlerCalled);
    }

    void shouldLookInAllChildrenReportedForAllChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);
        result->data();
        result = queries->findChildren(task1); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task2);
        QCOMPARE(result->data().at(1), task3);

        // Should not change nothing
        result = queries->findChildren(task1);

        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task2);
        QCOMPARE(result->data().at(1), task3);
    }

    void shouldNotCrashWhenWeAskAgainTheSameChildrenList()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // One task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);

        // We'll make the same queries twice
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob11 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob11->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob21 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob21->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob12 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob12->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob22 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob22->setItems(Akonadi::Item::List() << item1);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                          .thenReturn(itemFetchJob11)
                                                          .thenReturn(itemFetchJob12);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob21)
                                                           .thenReturn(itemFetchJob22);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));

        // The bug we're trying to hit here is the following:
        //  - when findChildren is called the first time a provider is created internally
        //  - result is deleted at the end of the loop, no one holds the provider with
        //    a strong reference anymore so it is deleted as well
        //  - when findChildren is called the second time, there's a risk of a dangling
        //    pointer if the recycling of providers is wrongly implemented which can lead
        //    to a crash, if it is properly done no crash will occur
        for (int i = 0; i < 2; i++) {
            // WHEN * 2
            Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);

            // THEN * 2
            QVERIFY(result->data().isEmpty());
            QTest::qWait(150);
            QVERIFY(result->data().isEmpty());
        }
    }

    void shouldReactToItemAddsForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // One task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);

        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                          .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning if task1 is parent of items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);
        QTest::qWait(150);
        QVERIFY(result->data().isEmpty());
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).exactly(1));

        // WHEN
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);

        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        monitor->addItem(item2);
        monitor->addItem(item3);

        // THEN
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data()[0], task2);
        QCOMPARE(result->data()[1], task3);
    }

    void shouldReactToItemChangesForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);

        bool replaceHandlerCalled = false;
        result->addPostReplaceHandler([&replaceHandlerCalled](const Domain::Task::Ptr &, int) {
                                          replaceHandlerCalled = true;
                                      });
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).thenReturn();
        monitor->changeItem(item2);

        // Then
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task2);
        QCOMPARE(result->data().at(1), task3);

        QVERIFY(replaceHandlerCalled);
    }

    void shouldRemoveItemFromCorrespondingResultWhenRelatedItemChangeForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(true)
                                                                                     .thenReturn(false)
                                                                                     .thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        monitor->changeItem(item2);

        // Then
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().at(0), task3);
    }

    void shouldAddItemToCorrespondingResultWhenRelatedItemChangeForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(false)
                                                                                     .thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);

        bool replaceHandlerCalled = false;
        result->addPostReplaceHandler([&replaceHandlerCalled](const Domain::Task::Ptr &, int) {
                                          replaceHandlerCalled = true;
                                      });
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 1);

        // WHEN
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn("1");
        monitor->changeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task3);
        QCOMPARE(result->data().at(1), task2);

        QVERIFY(!replaceHandlerCalled);
    }

    void shouldMoveItemToCorrespondingResultWhenRelatedItemChangeForChildTask()
    {
        // GIVEN

        // One top level collection
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Two top level tasks and one child task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        auto task1 = Domain::Task::Ptr::create();
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        auto task2 = Domain::Task::Ptr::create();
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        auto task3 = Domain::Task::Ptr::create();

        // Jobs
        auto collectionFetchJob1 = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob1->setCollections(Akonadi::Collection::List() << col);
        auto itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        auto collectionFetchJob2 = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob2->setCollections(Akonadi::Collection::List() << col);
        auto itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        auto itemFetchJob3 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob3->setItems(Akonadi::Item::List() << item1);
        auto itemFetchJob4 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob4->setItems(Akonadi::Item::List() << item2);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob1)
                                                                 .thenReturn(collectionFetchJob2);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob1)
                                                           .thenReturn(itemFetchJob2);
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                          .thenReturn(itemFetchJob3);
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item2)
                                                          .thenReturn(itemFetchJob4);

        // Serializer mock returning the objects from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task2).thenReturn(item2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item3).thenReturn(true);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true)
                                                                                     .thenReturn(false);

        // Serializer mock returning if task2 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item3).thenReturn(false)
                                                                                     .thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result1 = queries->findChildren(task1);
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result2 = queries->findChildren(task2);

        QTest::qWait(150);
        QCOMPARE(result1->data().size(), 1);
        QCOMPARE(result1->data().at(0), task3);
        QCOMPARE(result2->data().size(), 0);

        // WHEN
        monitor->changeItem(item3); // Now gets a different related id

        // Then
        QCOMPARE(result1->data().size(), 0);
        QCOMPARE(result2->data().size(), 1);
        QCOMPARE(result2->data().at(0), task3);
    }

    void shouldReactToItemRemovesForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task1, item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).thenReturn();
        monitor->removeItem(item2);

        // Then
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).exactly(1));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().at(0), task3);
    }

    void shouldLookInAllReportedForTopLevelTasks()
    {
        // GIVEN

        // Two top level collections
        Akonadi::Collection col1(42);
        col1.setParentCollection(Akonadi::Collection::root());
        Akonadi::Collection col2(43);
        col2.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col1 << col2);

        // One task in the first collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col1);
        Domain::Task::Ptr task1(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);

        // Two tasks in the second collection
        Akonadi::Item item2(43);
        item2.setParentCollection(col2);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col2);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        result->data();
        result = queries->findTopLevel(); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col1).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
    }

    void shouldReactToItemAddsForTopLevelTasks()
    {
        // GIVEN

        // Empty collection fetch
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        QTest::qWait(150);
        QVERIFY(result->data().isEmpty());

        // WHEN
        Akonadi::Item item1(42);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        Domain::Task::Ptr task2(new Domain::Task);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn("1");
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);

        monitor->addItem(item1);
        monitor->addItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(0));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().first(), task1);
    }

    void shouldReactToItemRemovesForTopLevelTasks()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        monitor->removeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(result->data().size(), 1);
    }

    void shouldReactToItemChangesForTopLevelTasks()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).thenReturn();

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        // Even though the pointer didn't change it's convenient to user if we call
        // the replace handlers
        bool replaceHandlerCalled = false;
        result->addPostReplaceHandler([&replaceHandlerCalled](const Domain::Task::Ptr &, int) {
                                          replaceHandlerCalled = true;
                                      });
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        monitor->changeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::updateTaskFromItem).when(task2, item2).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
        QVERIFY(replaceHandlerCalled);
    }

    void shouldRemoveItemFromTopLevelResultWhenRelatedItemChangeForTopLevelTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString()).thenReturn("1");
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        // WHEN
        monitor->changeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(result->data().size(), 1);
        QCOMPARE(result->data().at(0), task1);
    }

    void shouldAddItemToTopLevelResultWhenRelatedItemChangeForChildrenTask()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn("1").thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 1);

        // WHEN
        monitor->changeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
    }

    void shouldRemoveParentNodeAndMoveChildrenInTopLevelResult()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1 << item2 << item3);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob3 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob3->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob1)
                                                           .thenReturn(itemFetchJob3);
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task2).thenReturn(item2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item3).thenReturn(task3);

        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task1, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task2, item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::representsItem).when(task3, item2).thenReturn(false);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);

        // Serializer mock returning if task1 is parent of items
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item1).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item2).thenReturn(false);
        serializerMock(&Akonadi::SerializerInterface::isTaskChild).when(task2, item3).thenReturn(true);

        // Monitor mock
        auto monitor = Testlib::AkonadiFakeMonitor::Ptr::create();

        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             monitor));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        QTest::qWait(150);
        QCOMPARE(result->data().size(), 2);

        Domain::QueryResult<Domain::Task::Ptr>::Ptr resultChild = queries->findChildren(task2);
        QTest::qWait(150);
        QCOMPARE(resultChild->data().size(), 1);

        // WHEN
        monitor->removeItem(item2);

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(2));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(resultChild->data().size(), 0);
        QCOMPARE(result->data().size(), 1); // FIXME: Should become 2 once we got a proper cache in place
    }

    void shouldNotCrashDuringFindChildrenWhenJobIsKilled()
    {
        // GIVEN

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        itemFetchJob1->setExpectedError(KJob::KilledJobError);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));

        QCOMPARE(result->data().size(), 0);
    }


    void shouldNotCrashDuringFindChildrenWhenItemsJobReceiveResult_data()
    {
        QTest::addColumn<Akonadi::Item>("item1");
        QTest::addColumn<Testlib::AkonadiFakeItemFetchJob*>("itemFetchJob1");
        QTest::addColumn<Akonadi::Collection>("col");
        QTest::addColumn<Testlib::AkonadiFakeItemFetchJob*>("itemFetchJob2");
        QTest::addColumn<Domain::Task::Ptr>("task1");
        QTest::addColumn<bool>("deleteQuery");

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("No error with empty list") << item1 << itemFetchJob1 << col << itemFetchJob2 << task1 << false;

        itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setExpectedError(KJob::KilledJobError);
        QTest::newRow("Error with empty list") << item1 << itemFetchJob1 << col << itemFetchJob2 << task1 << true;

        itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);
        itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setExpectedError(KJob::KilledJobError);
        itemFetchJob2->setItems(Akonadi::Item::List() << item1 << item2 << item3);
        QTest::newRow("Error with list") <<  item1 << itemFetchJob1 << col << itemFetchJob2 << task1 << true;
    }

    void shouldNotCrashDuringFindChildrenWhenItemsJobReceiveResult()
    {
        // GIVEN
        QFETCH(Akonadi::Item, item1);
        QFETCH(Testlib::AkonadiFakeItemFetchJob*, itemFetchJob1);
        QFETCH(Akonadi::Collection, col);
        QFETCH(Testlib::AkonadiFakeItemFetchJob*, itemFetchJob2);
        QFETCH(Domain::Task::Ptr, task1);
        QFETCH(bool, deleteQuery);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromTask).when(task1).thenReturn(item1);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findChildren(task1);

        if (deleteQuery)
            delete queries.take();

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item1).exactly(1));

        QCOMPARE(result->data().size(), 0);
    }

    void shouldNotCrashDuringFindAllWhenFetchJobFailedOrEmpty_data()
    {
        QTest::addColumn<Akonadi::Collection>("col");
        QTest::addColumn<Testlib::AkonadiFakeCollectionFetchJob*>("collectionFetchJob");
        QTest::addColumn<Testlib::AkonadiFakeItemFetchJob*>("itemFetchJob");
        QTest::addColumn<bool>("deleteQuery");
        QTest::addColumn<bool>("fechItemsIsCalled");

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);

        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("No error with empty collection list") << col << collectionFetchJob << itemFetchJob << false << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setExpectedError(KJob::KilledJobError);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("Error with empty collection list") << col << collectionFetchJob << itemFetchJob << true << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setExpectedError(KJob::KilledJobError);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("Error with collection list") <<  col << collectionFetchJob << itemFetchJob  << true << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("No error with empty item list") << col << collectionFetchJob << itemFetchJob << false << true;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);

        QTest::newRow("Error with empty item list") << col << collectionFetchJob << itemFetchJob << true << true;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        QTest::newRow("Error with item list") << col << collectionFetchJob << itemFetchJob << true << true;
    }

    void shouldNotCrashDuringFindAllWhenFetchJobFailedOrEmpty()
    {
        // GIVEN
        QFETCH(Akonadi::Collection, col);
        QFETCH(Testlib::AkonadiFakeCollectionFetchJob*, collectionFetchJob);
        QFETCH(Testlib::AkonadiFakeItemFetchJob*, itemFetchJob);
        QFETCH(bool, deleteQuery);
        QFETCH(bool, fechItemsIsCalled);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findAll();

        if (deleteQuery)
            delete queries.take();

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        if (fechItemsIsCalled)
            QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));

        QCOMPARE(result->data().size(), 0);
    }

    void shouldNotCrashDuringFindTopLevelWhenFetchJobFailedOrEmpty_data()
    {
        QTest::addColumn<Akonadi::Collection>("col");
        QTest::addColumn<Testlib::AkonadiFakeCollectionFetchJob*>("collectionFetchJob");
        QTest::addColumn<Testlib::AkonadiFakeItemFetchJob*>("itemFetchJob");
        QTest::addColumn<bool>("deleteQuery");
        QTest::addColumn<bool>("fechItemsIsCalled");

        // One top level collections
        Akonadi::Collection col(42);
        col.setParentCollection(Akonadi::Collection::root());

        // Three task in the collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col);
        Domain::Task::Ptr task1(new Domain::Task);
        Akonadi::Item item2(43);
        item2.setParentCollection(col);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col);
        Domain::Task::Ptr task3(new Domain::Task);

        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("No error with empty collection list") << col << collectionFetchJob << itemFetchJob << false << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setExpectedError(KJob::KilledJobError);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("Error with empty collection list") << col << collectionFetchJob << itemFetchJob << true << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setExpectedError(KJob::KilledJobError);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("Error with collection list") <<  col << collectionFetchJob << itemFetchJob  << true << false;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);

        QTest::newRow("No error with empty item list") << col << collectionFetchJob << itemFetchJob << false << true;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);

        QTest::newRow("Error with empty item list") << col << collectionFetchJob << itemFetchJob << true << true;

        collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col);
        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);
        itemFetchJob->setItems(Akonadi::Item::List() << item1 << item2 << item3);

        QTest::newRow("Error with item list") << col << collectionFetchJob << itemFetchJob << true << true;
    }

    void shouldNotCrashDuringFindTopLevelWhenFetchJobFailedOrEmpty()
    {
        // GIVEN
        QFETCH(Akonadi::Collection, col);
        QFETCH(Testlib::AkonadiFakeCollectionFetchJob*, collectionFetchJob);
        QFETCH(Testlib::AkonadiFakeItemFetchJob*, itemFetchJob);
        QFETCH(bool, deleteQuery);
        QFETCH(bool, fechItemsIsCalled);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col)
                                                           .thenReturn(itemFetchJob);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();

        if (deleteQuery)
            delete queries.take();

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        if (fechItemsIsCalled)
            QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col).exactly(1));

        QCOMPARE(result->data().size(), 0);
    }

    void shouldIgnoreProjectsWhenReportingTopLevelTasks()
    {
        // GIVEN

        // Two top level collections
        Akonadi::Collection col1(42);
        col1.setParentCollection(Akonadi::Collection::root());
        Akonadi::Collection col2(43);
        col2.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col1 << col2);

        // One task in the first collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col1);
        Domain::Task::Ptr task1(new Domain::Task);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);

        // Two tasks and one project in the second collection
        Akonadi::Item item2(43);
        item2.setParentCollection(col2);
        Domain::Task::Ptr task2(new Domain::Task);
        Akonadi::Item item3(44);
        item3.setParentCollection(col2);
        Domain::Task::Ptr task3(new Domain::Task);
        Akonadi::Item item4(45);
        item4.setParentCollection(col2);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2 << item3 << item4);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        // Serializer mock returning if the item has a relatedItem
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item1).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item2).thenReturn(QString());
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item3).thenReturn("2");
        serializerMock(&Akonadi::SerializerInterface::relatedUidFromItem).when(item4).thenReturn(QString());

        // Serializer mock returning if the item is a task
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item3).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item4).thenReturn(false);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findTopLevel();
        result->data();
        result = queries->findTopLevel(); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col1).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col2).exactly(1));

        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(1));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
    }

    void shouldLookInAllWorkdayReportedForAllTasks_data()
    {
        QTest::addColumn<Domain::Task::Ptr>("task2");
        QTest::addColumn<bool>("isExpectedInWorkday");

        const auto today = Utils::DateTime::currentDateTime();

        Domain::Task::Ptr todayTask(new Domain::Task);
        todayTask->setStartDate(today);
        todayTask->setDueDate(today);
        QTest::newRow("todayTask") << todayTask << true;

        Domain::Task::Ptr pastTask(new Domain::Task);
        pastTask->setStartDate(today.addDays(-42));
        pastTask->setDueDate(today.addDays(-41));
        QTest::newRow("pastTask") << pastTask << true;

        Domain::Task::Ptr startTodayTask(new Domain::Task);
        startTodayTask->setStartDate(today);
        QTest::newRow("startTodayTask") << startTodayTask << true;

        Domain::Task::Ptr endTodayTask(new Domain::Task);
        endTodayTask->setDueDate(today);
        QTest::newRow("endTodayTask") << endTodayTask << true;

        Domain::Task::Ptr futureTask(new Domain::Task);
        futureTask->setStartDate(today.addDays(41));
        futureTask->setDueDate(today.addDays(42));
        QTest::newRow("futureTask") << futureTask << false;

        Domain::Task::Ptr pastDoneTask(new Domain::Task);
        pastDoneTask->setStartDate(today.addDays(-42));
        pastDoneTask->setDueDate(today.addDays(-41));
        pastDoneTask->setDone(true);
        pastDoneTask->setDoneDate(today.addDays(-30));
        QTest::newRow("pastDoneTask") << pastDoneTask << false;

        Domain::Task::Ptr todayDoneTask(new Domain::Task);
        todayDoneTask->setStartDate(today);
        todayDoneTask->setDueDate(today);
        todayDoneTask->setDone(true);
        QTest::newRow("todayDoneTask") << todayDoneTask << true;

        Domain::Task::Ptr startTodayDoneTask(new Domain::Task);
        startTodayDoneTask->setStartDate(today);
        startTodayDoneTask->setDone(true);
        QTest::newRow("startTodayDoneTask") << startTodayDoneTask << true;

        Domain::Task::Ptr endTodayDoneTask(new Domain::Task);
        endTodayDoneTask->setDueDate(today);
        endTodayDoneTask->setDone(true);
        QTest::newRow("endTodayDoneTask") << endTodayDoneTask << true;
    }

    void shouldLookInAllWorkdayReportedForAllTasks()
    {
        // GIVEN
        const auto today = Utils::DateTime::currentDateTime();

        // Two top level collections
        Akonadi::Collection col1(42);
        col1.setParentCollection(Akonadi::Collection::root());
        Akonadi::Collection col2(43);
        col2.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col1 << col2);

        // One task in the first collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col1);
        Domain::Task::Ptr task1(new Domain::Task);
        task1->setStartDate(today.addSecs(300));
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);

        // One task in the second collection (from data driven set)
        Akonadi::Item item2(43);
        item2.setParentCollection(col2);
        QFETCH(Domain::Task::Ptr, task2);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findWorkdayTopLevel();
        result->data();
        result = queries->findWorkdayTopLevel(); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col1).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col2).exactly(1));

        QFETCH(bool, isExpectedInWorkday);

        const int sizeExpected = (isExpectedInWorkday) ? 2 : 1;
        const int createTaskFromItemExpected = (isExpectedInWorkday) ? 2 : 1;

        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(2));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(createTaskFromItemExpected));

        QCOMPARE(result->data().size(), sizeExpected);
        QCOMPARE(result->data().at(0), task1);

        if (isExpectedInWorkday)
            QCOMPARE(result->data().at(1), task2);
    }

    void shouldLookInAllWorkdayReportedForAllTasksWhenOverrideDate()
    {
        // GIVEN
        qputenv("ZANSHIN_OVERRIDE_DATETIME", "2015-03-10");
        const auto today = Utils::DateTime::currentDateTime();

        // Two top level collections
        Akonadi::Collection col1(42);
        col1.setParentCollection(Akonadi::Collection::root());
        Akonadi::Collection col2(43);
        col2.setParentCollection(Akonadi::Collection::root());
        Testlib::AkonadiFakeCollectionFetchJob *collectionFetchJob = new Testlib::AkonadiFakeCollectionFetchJob(this);
        collectionFetchJob->setCollections(Akonadi::Collection::List() << col1 << col2);

        // One task in the first collection
        Akonadi::Item item1(42);
        item1.setParentCollection(col1);
        Domain::Task::Ptr task1(new Domain::Task);
        task1->setStartDate(today);
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob1 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob1->setItems(Akonadi::Item::List() << item1);

        // One task in the second collection
        Akonadi::Item item2(43);
        item2.setParentCollection(col2);
        Domain::Task::Ptr task2(new Domain::Task);
        task2->setStartDate(today.addSecs(3600));
        Testlib::AkonadiFakeItemFetchJob *itemFetchJob2 = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob2->setItems(Akonadi::Item::List() << item2);

        // Storage mock returning the fetch jobs
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                       Akonadi::StorageInterface::Recursive,
                                                                       Akonadi::StorageInterface::Tasks)
                                                                 .thenReturn(collectionFetchJob);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col1)
                                                           .thenReturn(itemFetchJob1);
        storageMock(&Akonadi::StorageInterface::fetchItems).when(col2)
                                                           .thenReturn(itemFetchJob2);

        // Serializer mock returning the tasks from the items
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item1).thenReturn(true);
        serializerMock(&Akonadi::SerializerInterface::isTaskItem).when(item2).thenReturn(true);

        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).thenReturn(task1);
        serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).thenReturn(task2);

        // WHEN
        QScopedPointer<Domain::TaskQueries> queries(new Akonadi::TaskQueries(storageMock.getInstance(),
                                                                             serializerMock.getInstance(),
                                                                             Testlib::AkonadiFakeMonitor::Ptr::create()));
        Domain::QueryResult<Domain::Task::Ptr>::Ptr result = queries->findWorkdayTopLevel();
        result->data();
        result = queries->findWorkdayTopLevel(); // Should not cause any problem or wrong data

        // THEN
        QVERIFY(result->data().isEmpty());
        QTest::qWait(150);
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchCollections).when(Akonadi::Collection::root(),
                                                                               Akonadi::StorageInterface::Recursive,
                                                                               Akonadi::StorageInterface::Tasks)
                                                                         .exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col1).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItems).when(col2).exactly(1));

        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item1).exactly(2));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createTaskFromItem).when(item2).exactly(2));

        QCOMPARE(result->data().size(), 2);
        QCOMPARE(result->data().at(0), task1);
        QCOMPARE(result->data().at(1), task2);
    }

};

QTEST_MAIN(AkonadiTaskQueriesTest)

#include "akonaditaskqueriestest.moc"
