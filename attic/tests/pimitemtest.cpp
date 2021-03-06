/*
   Copyright 2013 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include <qtest_kde.h>

#include <core/akonadiincidenceitem.h>
#include <core/pimitemfactory.h>
#include <core/akonadinoteitem.h>

Q_DECLARE_METATYPE(QModelIndex)

class PimItemTest : public QObject
{
    Q_OBJECT
    
private slots:
    void testGetAndSet_data()
    {
        QTest::addColumn<AkonadiBaseItem::Ptr>("item");
        QTest::newRow("event") << AkonadiBaseItem::Ptr(new AkonadiIncidenceItem(PimItem::Event));
        QTest::newRow("note") << AkonadiBaseItem::Ptr(new AkonadiNoteItem());
    }
    
    //Ensure save and load from akonadi item works
    void testGetAndSet()
    {
        QFETCH(AkonadiBaseItem::Ptr, item);
        item->setTitle("title");
        item->setText("text");
        
        Akonadi::Item akonadiItem = item->getItem();
        akonadiItem.setId(1);
        
        PimItem::Ptr pimitem = PimItemFactory::getItem(akonadiItem);
        QCOMPARE(pimitem->uid(), item->uid());
        QCOMPARE(pimitem->title(), item->title());
        QCOMPARE(pimitem->text(), item->text());
    }
    
    void testUidConsistency_data()
    {
        QTest::addColumn<PimItem::Ptr>("item");
        QTest::newRow("event") << PimItem::Ptr(new AkonadiIncidenceItem(PimItem::Event));
        QTest::newRow("note") << PimItem::Ptr(new AkonadiNoteItem());
    }
    
    void testUidConsistency()
    {
        QFETCH(PimItem::Ptr, item);
        const QString &uid = item->uid();
        QVERIFY(!uid.isEmpty());
        QCOMPARE(item->uid(), uid);
    }
};

QTEST_KDEMAIN(PimItemTest, GUI)

#include "pimitemtest.moc"
