/* This file is part of Zanshin

   Copyright 2015 Kevin Ottens <ervin@kde.org>

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

#include "testlib/akonadifakedata.h"
#include "akonadi/akonadimonitorinterface.h"

#include <QtTest>

namespace QTest {

template<typename T>
inline bool zCompareHelper(bool isOk,
                           const T &left, const T &right,
                           const char *actual, const char *expected,
                           const char *file, int line)
{
    return isOk
         ? compare_helper(true, "COMPARE()", toString<T>(left), toString<T>(right), actual, expected, file, line)
         : compare_helper(false, "Compared values are not the same",
                          toString<T>(left), toString<T>(right),
                          actual, expected,
                          file, line);
}

// More aggressive compare to make sure we just don't get collections with ids out
template <>
inline bool qCompare(const Akonadi::Collection &left, const Akonadi::Collection &right,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    return zCompareHelper((left == right) && (left.displayName() == right.displayName()),
                          left, right, actual, expected, file, line);
}

// More aggressive compare to make sure we just don't get tags with ids out
template <>
inline bool qCompare(const Akonadi::Tag &left, const Akonadi::Tag &right,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    return zCompareHelper((left == right) && (left.name() == right.name()),
                          left, right, actual, expected, file, line);
}

// More aggressive compare to make sure we just don't get items with ids out
template <>
inline bool qCompare(const Akonadi::Item &left, const Akonadi::Item &right,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    return zCompareHelper((left == right) && (left.payloadData() == right.payloadData()),
                          left, right, actual, expected, file, line);
}

}

class AkonadiFakeDataTest : public QObject
{
    Q_OBJECT
public:
    explicit AkonadiFakeDataTest(QObject *parent = Q_NULLPTR)
        : QObject(parent)
    {
        qRegisterMetaType<Akonadi::Collection>();
        qRegisterMetaType<Akonadi::Tag>();
        qRegisterMetaType<Akonadi::Item>();
    }

private slots:
    void shouldBeInitiallyEmpty()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();

        // THEN
        QVERIFY(data.collections().isEmpty());
        QVERIFY(data.tags().isEmpty());
        QVERIFY(data.items().isEmpty());
    }

    void shouldCreateCollections()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(collectionAdded(Akonadi::Collection)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        auto c2 = Akonadi::Collection(43);
        c2.setName("43");
        const auto colSet = QSet<Akonadi::Collection>() << c1 << c2;

        // WHEN
        data.createCollection(c1);
        data.createCollection(c2);

        // THEN
        QCOMPARE(data.collections().toSet(), colSet);
        QCOMPARE(data.collection(c1.id()), c1);
        QCOMPARE(data.collection(c2.id()), c2);

        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Collection>(), c1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Collection>(), c2);
    }

    void shouldModifyCollections()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(collectionChanged(Akonadi::Collection)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto c2 = Akonadi::Collection(c1.id());
        c2.setName("42-bis");

        // WHEN
        data.modifyCollection(c2);

        // THEN
        QCOMPARE(data.collections().size(), 1);
        QCOMPARE(data.collection(c1.id()), c2);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Collection>(), c2);
    }

    void shouldListChildCollections()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        auto c2 = Akonadi::Collection(43);
        c2.setName("43");
        c2.setParentCollection(Akonadi::Collection(42));
        const auto colSet = QSet<Akonadi::Collection>() << c2;

        // WHEN
        data.createCollection(c1);
        data.createCollection(c2);

        // THEN
        QVERIFY(data.childCollections(c2.id()).isEmpty());
        QCOMPARE(data.childCollections(c1.id()).toSet(), colSet);
    }

    void shouldReparentCollectionsOnModify()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto c2 = Akonadi::Collection(43);
        c2.setName("43");
        data.createCollection(c2);

        auto c3 = Akonadi::Collection(44);
        c3.setParentCollection(Akonadi::Collection(42));
        data.createCollection(c3);

        // WHEN
        c3.setParentCollection(Akonadi::Collection(43));
        data.modifyCollection(c3);

        // THEN
        QVERIFY(data.childCollections(c1.id()).isEmpty());
        QCOMPARE(data.childCollections(c2.id()).size(), 1);
        QCOMPARE(data.childCollections(c2.id()).first(), c3);
    }

    void shouldRemoveCollections()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(collectionRemoved(Akonadi::Collection)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto c2 = Akonadi::Collection(43);
        c2.setName("43");
        c2.setParentCollection(Akonadi::Collection(42));
        data.createCollection(c2);

        auto c3 = Akonadi::Collection(44);
        c3.setName("44");
        c3.setParentCollection(Akonadi::Collection(43));
        data.createCollection(c3);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(Akonadi::Collection(43));
        data.createItem(i1);

        auto i2 = Akonadi::Item(43);
        i2.setPayloadFromData("43");
        i2.setParentCollection(Akonadi::Collection(44));
        data.createItem(i2);

        // WHEN
        data.removeCollection(c2);

        // THEN
        QCOMPARE(data.collections().size(), 1);
        QCOMPARE(data.collections().first(), c1);

        QVERIFY(!data.collection(c2.id()).isValid());
        QVERIFY(!data.collection(c3.id()).isValid());

        QVERIFY(data.childCollections(c1.id()).isEmpty());
        QVERIFY(data.childCollections(c2.id()).isEmpty());
        QVERIFY(data.childCollections(c3.id()).isEmpty());

        QVERIFY(data.items().isEmpty());

        QVERIFY(!data.item(i1.id()).isValid());
        QVERIFY(!data.item(i2.id()).isValid());

        QVERIFY(data.childItems(c2.id()).isEmpty());
        QVERIFY(data.childItems(c3.id()).isEmpty());

        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Collection>(), c3);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Collection>(), c2);
    }

    void shouldCreateTags()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(tagAdded(Akonadi::Tag)));

        auto t1 = Akonadi::Tag(42);
        t1.setName("42");
        auto t2 = Akonadi::Tag(43);
        t2.setName("43");

        // WHEN
        data.createTag(t1);
        data.createTag(t2);

        // THEN
        QCOMPARE(data.tags().size(), 2);
        QVERIFY(data.tags().contains(t1));
        QVERIFY(data.tags().contains(t2));
        QCOMPARE(data.tag(t1.id()), t1);
        QCOMPARE(data.tag(t2.id()), t2);

        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Tag>(), t1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Tag>(), t2);
    }

    void shouldModifyTags()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(tagChanged(Akonadi::Tag)));

        auto t1 = Akonadi::Tag(42);
        t1.setName("42");
        data.createTag(t1);

        auto t2 = Akonadi::Tag(t1.id());
        t2.setName("42-bis");

        // WHEN
        data.modifyTag(t2);

        // THEN
        QCOMPARE(data.tags().size(), 1);
        QCOMPARE(data.tag(t1.id()), t2);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Tag>(), t2);
    }

    void shouldRemoveTags()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy tagSpy(monitor.data(), SIGNAL(tagRemoved(Akonadi::Tag)));
        QSignalSpy itemSpy(monitor.data(), SIGNAL(itemChanged(Akonadi::Item)));

        auto c1 = Akonadi::Collection(42);
        data.createCollection(c1);

        auto t1 = Akonadi::Tag(42);
        t1.setName("42");
        data.createTag(t1);

        auto t2 = Akonadi::Tag(43);
        t2.setName("43");
        data.createTag(t2);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(c1);
        i1.setTag(Akonadi::Tag(t1.id()));
        data.createItem(i1);

        auto i2 = Akonadi::Item(43);
        i2.setPayloadFromData("43");
        i2.setParentCollection(c1);
        i2.setTag(Akonadi::Tag(t2.id()));
        data.createItem(i2);

        const auto itemSet = QSet<Akonadi::Item>() << i1 << i2;

        // WHEN
        data.removeTag(t2);

        // THEN
        QCOMPARE(data.tags().size(), 1);
        QCOMPARE(data.tags().first(), t1);

        QVERIFY(!data.tag(t2.id()).isValid());

        QCOMPARE(data.tagItems(t1.id()).size(), 1);
        QCOMPARE(data.tagItems(t1.id()).first(), i1);
        QVERIFY(data.tagItems(t2.id()).isEmpty());

        QCOMPARE(data.items().toSet(), itemSet);

        QVERIFY(data.item(i1.id()).isValid());
        QVERIFY(data.item(i2.id()).isValid());
        QVERIFY(!data.item(i2.id()).tags().contains(t2));

        QCOMPARE(tagSpy.size(), 1);
        QCOMPARE(tagSpy.takeFirst().takeFirst().value<Akonadi::Tag>(), t2);

        QCOMPARE(itemSpy.size(), 1);
        QCOMPARE(itemSpy.first().first().value<Akonadi::Item>(), i2);
        QVERIFY(!itemSpy.first().first().value<Akonadi::Item>().tags().contains(t2));
    }

    void shouldCreateItems()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(itemAdded(Akonadi::Item)));

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        auto i2 = Akonadi::Item(43);
        i2.setPayloadFromData("43");
        const auto itemSet = QSet<Akonadi::Item>() << i1 << i2;

        // WHEN
        data.createItem(i1);
        data.createItem(i2);

        // THEN
        QCOMPARE(data.items().toSet(), itemSet);
        QCOMPARE(data.item(i1.id()), i1);
        QCOMPARE(data.item(i2.id()), i2);

        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i2);
    }

    void shouldModifyItems()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(itemChanged(Akonadi::Item)));
        QSignalSpy moveSpy(monitor.data(), SIGNAL(itemMoved(Akonadi::Item)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(Akonadi::Collection(42));
        data.createItem(i1);

        auto i2 = Akonadi::Item(i1.id());
        i2.setPayloadFromData("42-bis");
        i2.setParentCollection(Akonadi::Collection(42));

        // WHEN
        data.modifyItem(i2);

        // THEN
        QCOMPARE(data.items().size(), 1);
        QCOMPARE(data.item(i1.id()), i2);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i2);

        QCOMPARE(moveSpy.size(), 0);
    }

    void shouldListChildItems()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(Akonadi::Collection(42));

        // WHEN
        data.createItem(i1);

        // THEN
        QCOMPARE(data.childItems(c1.id()).size(), 1);
        QCOMPARE(data.childItems(c1.id()).first(), i1);
    }

    void shouldReparentItemsOnModify()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(itemMoved(Akonadi::Item)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto c2 = Akonadi::Collection(43);
        c2.setName("43");
        data.createCollection(c2);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(Akonadi::Collection(42));
        data.createItem(i1);

        // WHEN
        i1.setPayloadFromData("42-bis");
        i1.setParentCollection(Akonadi::Collection(43));
        data.modifyItem(i1);

        // THEN
        QVERIFY(data.childItems(c1.id()).isEmpty());
        QCOMPARE(data.childItems(c2.id()).size(), 1);
        QCOMPARE(data.childItems(c2.id()).first(), i1);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i1);
    }

    void shouldListTagItems()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        auto t1 = Akonadi::Tag(42);
        t1.setName("42");
        data.createTag(t1);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setTag(Akonadi::Tag(42));

        // WHEN
        data.createItem(i1);

        // THEN
        QCOMPARE(data.tagItems(t1.id()).size(), 1);
        QCOMPARE(data.tagItems(t1.id()).first(), i1);
    }

    void shouldRetagItemsOnModify()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(itemChanged(Akonadi::Item)));

        auto t1 = Akonadi::Tag(42);
        t1.setName("42");
        data.createTag(t1);

        auto t2 = Akonadi::Tag(43);
        t2.setName("43");
        data.createTag(t2);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setTag(Akonadi::Tag(42));
        data.createItem(i1);

        // WHEN
        i1.setPayloadFromData("42-bis");
        i1.clearTag(Akonadi::Tag(42));
        i1.setTag(Akonadi::Tag(43));
        data.modifyItem(i1);

        // THEN
        QVERIFY(data.tagItems(t1.id()).isEmpty());
        QCOMPARE(data.tagItems(t2.id()).size(), 1);
        QCOMPARE(data.tagItems(t2.id()).first(), i1);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i1);
    }

    void shouldRemoveItems()
    {
        // GIVEN
        auto data = Testlib::AkonadiFakeData();
        QScopedPointer<Akonadi::MonitorInterface> monitor(data.createMonitor());
        QSignalSpy spy(monitor.data(), SIGNAL(itemRemoved(Akonadi::Item)));

        auto c1 = Akonadi::Collection(42);
        c1.setName("42");
        data.createCollection(c1);

        auto i1 = Akonadi::Item(42);
        i1.setPayloadFromData("42");
        i1.setParentCollection(Akonadi::Collection(42));
        data.createItem(i1);

        // WHEN
        data.removeItem(i1);

        // THEN
        QVERIFY(data.items().isEmpty());
        QVERIFY(!data.item(i1.id()).isValid());
        QVERIFY(data.childItems(c1.id()).isEmpty());

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.takeFirst().takeFirst().value<Akonadi::Item>(), i1);
    }
};

QTEST_MAIN(AkonadiFakeDataTest)

#include "akonadifakedatatest.moc"
