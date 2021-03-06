/* This file is part of Zanshin Todo.

   Copyright 2011 Mario Bensi <mbensi@ipsquad.net>

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

#include <QtGui/QStandardItemModel>

#include "core/todometadatamodel.h"
#include "testlib/mockdatastore.h"
#include "testlib/testlib.h"
#include "testlib/modelbuilderbehavior.h"

#include <KDE/Akonadi/EntityTreeModel>

using namespace Zanshin::Test;

Q_DECLARE_METATYPE(QModelIndex)

/*
 * Test for the TodoMetadataModel
 */
class TodoMetadataModelTest : public QObject
{
    Q_OBJECT
private:
    static QModelIndexList collectChildren(QAbstractItemModel *model, const QModelIndex &root)
    {
        QModelIndexList result;

        for (int row=0; row<model->rowCount(root); row++) {
            QModelIndex child = model->index(row, 0, root);

            if (!child.isValid()) {
                return result;
            }

            result+= collectChildren(model, child);
            result << child;
        }

        return result;
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<QModelIndex>();
        QList<int> roles;
        roles << Qt::DisplayRole
              << Akonadi::EntityTreeModel::ItemRole
              << Akonadi::EntityTreeModel::CollectionRole;

        QTest::setEvaluatedItemRoles(roles);
        DataStoreInterface::overrideImplementation(new MockDataStore);
    }

    void shouldRememberItsSourceModel()
    {
        //GIVEN
        QStandardItemModel baseModel;
        TodoMetadataModel todoMetadataModel(&baseModel);
        ModelTest mt(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&baseModel);

        //THEN
        QVERIFY(todoMetadataModel.sourceModel() == &baseModel);
    }

    void shouldRetrieveItemState_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<QVariant>( "state" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1", InProgress);
        T t2(2, 1, "t2", QString(), "t2", Done);
        T t3(3, 1, "t3", QString(), "t3", Done, ProjectTag);

        ModelStructure sourceStructure;
        sourceStructure << t1;

        QVariant state = Qt::Unchecked;

        QTest::newRow( "check state role in progress" ) << sourceStructure << state;

        sourceStructure.clear();
        sourceStructure << t2;

        state = Qt::Checked;
        QTest::newRow( "check state role done" ) << sourceStructure << state;

        sourceStructure.clear();
        sourceStructure << t3;

        state = QVariant();
        QTest::newRow( "check state role on project" ) << sourceStructure << state;

        sourceStructure.clear();
        sourceStructure << c1;

        state = QVariant();
        QTest::newRow( "check state role on collection" ) << sourceStructure << state;
    }

    void shouldRetrieveItemState()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel todoMetadataModel;
        ModelTest t1(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&source);

        //THEN
        QFETCH(QVariant, state);
        QModelIndex index = todoMetadataModel.index(0,0);

        QCOMPARE(index.data(Qt::CheckStateRole), state);
    }

    void shouldRetrieveItemUid_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<QString>( "uid" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        QString uid;
        QTest::newRow( "check uid of a collection" ) << sourceStructure << uid;

        sourceStructure.clear();
        sourceStructure << t1;

        uid = "t1";
        QTest::newRow( "check uid for a todo" ) << sourceStructure << uid;
    }

    void shouldRetrieveItemUid()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel todoMetadataModel;
        ModelTest t1(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&source);

        //THEN
        QFETCH(QString, uid);
        QModelIndex index = todoMetadataModel.index(0,0);

        QCOMPARE(index.data(Zanshin::UidRole).toString(), uid);
    }

    void shouldRetrieveItemParentUid_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<QString>( "parentUid" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1");
        T t2(2, 1, "t2", "t1", "t2");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        QString parentUid;
        QTest::newRow( "check without parent" ) << sourceStructure << parentUid;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1;

        QTest::newRow( "check parentUid with collection as parent" ) << sourceStructure << parentUid;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2;

        parentUid = "t1";

        QTest::newRow( "check parentUid with todo as parent" ) << sourceStructure << parentUid;
    }

    void shouldRetrieveItemParentUid()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel todoMetadataModel;
        ModelTest t1(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&source);

        //THEN
        QFETCH(QString, parentUid);
        QModelIndex index = todoMetadataModel.index(0,0);
        while (todoMetadataModel.rowCount(index) > 0) {
            index = todoMetadataModel.index(todoMetadataModel.rowCount(index)-1, 0, index);
        }
        if (!parentUid.isEmpty()) {
            QVERIFY(index.data(Zanshin::ParentUidRole).toStringList().size() == 1);
            QCOMPARE(index.data(Zanshin::ParentUidRole).toStringList().first(), parentUid);
        } else {
            QVERIFY(index.data(Zanshin::ParentUidRole).toStringList().isEmpty());
        }
    }

    void shouldRetrieveItemType_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<int>( "type" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1", InProgress, ProjectTag);
        T t2(2, 1, "t2", "t1", "t2");
        T t3(3, 2, "t3", "t3", "t3");
        V inbox(Inbox);
        V contextRoot(Contexts);
        V nocontext(NoContext);
        Cat context("cat");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        int type = Zanshin::Collection;

        QTest::newRow( "check collection type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << t1;

        type = Zanshin::ProjectTodo;

        QTest::newRow( "check project type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << t2;

        type = Zanshin::StandardTodo;

        QTest::newRow( "check todo type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << t1
                        << t2;

        type = Zanshin::ProjectTodo;

        QTest::newRow( "check todo type when it has a child" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << inbox;

        type = 0;

        QTest::newRow( "check inbox type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << contextRoot;

        type = 0;

        QTest::newRow( "check context root type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << nocontext;

        type = 0;

        QTest::newRow( "check no context type" ) << sourceStructure << type;

        sourceStructure.clear();
        sourceStructure << context;

        type = 0;

        QTest::newRow( "check context type" ) << sourceStructure << type;
    }

    void shouldRetrieveItemType()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel todoMetadataModel;
        ModelTest t1(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&source);

        //THEN
        QFETCH(int, type);
        QModelIndex index = todoMetadataModel.index(0,0);

        QCOMPARE(index.data(Zanshin::ItemTypeRole).toInt(), type);
    }

    void shouldRetrieveItemContexts_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<QStringList>( "contexts" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1");
        T t2(2, 1, "t2", "t1", "t2", Done, NoTag, QString(), "cat1");
        T t3(3, 2, "t3", "t2", "t3", Done, NoTag, QString(), "cat2");
        T t4(4, 3, "t4", "t3", "t4", Done, NoTag, QString(), "cat2");
        T t5(5, 4, "t5", "t4", "t5", Done, NoTag, QString(), "cat3");
        T t6(6, 5, "t6", "t5", "t6", Done, NoTag, QString(), "cat3");
        T t7(7, 6, "t7", "t6", "t7", Done, NoTag, QString(), "cat4");
        T t8(8, 7, "t8", "t7", "t8", Done, NoTag, QString(), "cat5");
        T t9(9, 8, "t9", "t8", "t9");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        QStringList contexts;
        QTest::newRow( "get context list on collection" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1;

        QTest::newRow( "check todo without context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2;

        contexts << "cat1";

        QTest::newRow( "check todo with context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3;

        contexts.clear();
        contexts << "cat1"
                   << "cat2";

        QTest::newRow( "check todo with the different context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3
                        << _+t4;

        contexts.clear();
        contexts << "cat2"
                   << "cat1";

        QTest::newRow( "check todo with the same context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3
                        << _+t4
                        << _+t5
                        << _+t6
                        << _+t7
                        << _+t8
                        << _+t9;

        contexts.clear();
        contexts << "cat5"
                   << "cat4"
                   << "cat3"
                   << "cat2"
                   << "cat1";

        QTest::newRow( "check todo with some context duplicate" ) << sourceStructure << contexts;
    }

    void shouldRetrieveItemAncestorsContexts_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<QStringList>( "contexts" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1");
        T t2(2, 1, "t2", "t1", "t2", Done, NoTag, QString(), "cat1");
        T t3(3, 2, "t3", "t2", "t3", Done, NoTag, QString(), "cat2");
        T t4(4, 3, "t4", "t3", "t4", Done, NoTag, QString(), "cat2");
        T t5(5, 4, "t5", "t4", "t5", Done, NoTag, QString(), "cat3");
        T t6(6, 5, "t6", "t5", "t6", Done, NoTag, QString(), "cat3");
        T t7(7, 6, "t7", "t6", "t7", Done, NoTag, QString(), "cat4");
        T t8(8, 7, "t8", "t7", "t8", Done, NoTag, QString(), "cat5");
        T t9(9, 8, "t9", "t8", "t9");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        QStringList contexts;
        QTest::newRow( "get context list on collection" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1;

        QTest::newRow( "check todo without context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2;

        QTest::newRow( "check todo with context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3;

        contexts << "cat1";

        QTest::newRow( "check todo with the different context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3
                        << _+t4;

        contexts.clear();
        contexts << "cat2"
                   << "cat1";

        QTest::newRow( "check todo with the same context set" ) << sourceStructure << contexts;

        sourceStructure.clear();
        sourceStructure << c1
                        << _+t1
                        << _+t2
                        << _+t3
                        << _+t4
                        << _+t5
                        << _+t6
                        << _+t7
                        << _+t8
                        << _+t9;

        contexts.clear();
        contexts << "cat5"
                   << "cat4"
                   << "cat3"
                   << "cat2"
                   << "cat1";

        QTest::newRow( "check todo with some context duplicate" ) << sourceStructure << contexts;
    }

    void shouldRetrieveItemFlags_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<int>( "column" );
        QTest::addColumn<int>( "flags" );

        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1", InProgress, ProjectTag);
        T t2(2, 1, "t2", "t1", "t2");
        V inbox(Inbox);
        V contextRoot(Contexts);
        V nocontext(NoContext);
        Cat context("cat");

        ModelStructure sourceStructure;
        sourceStructure << c1;

        int flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
        int column = 0;
        QTest::newRow( "get flags on collection" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << t2;

        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on todo" ) << sourceStructure << column << flags;

        flags = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
        column = 4;

        QTest::newRow( "get flags on todo on column 4" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << t1;

        column = 0;
        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on project" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << inbox;

        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on inbox" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << context;

        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on context" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << contextRoot;

        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on context root" ) << sourceStructure << column << flags;

        sourceStructure.clear();
        sourceStructure << nocontext;

        flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        QTest::newRow( "get flags on NoContext" ) << sourceStructure << column << flags;
    }

    void shouldRetrieveItemFlags()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel todoMetadataModel;
        ModelTest t1(&todoMetadataModel);

        //WHEN
        todoMetadataModel.setSourceModel(&source);

        //THEN
        QFETCH(int, column);
        QFETCH(int, flags);

        QModelIndex index = todoMetadataModel.index(0,column);

        QCOMPARE(todoMetadataModel.flags(index), flags);
    }


    void shouldNotCrashWhenParentBecomesEmpty_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<ModelPath>( "itemToTest" );
        QTest::addColumn<ModelPath>( "itemToRemove" );
        //QTest::addColumn<ModelStructure>( "outputStructure" );

        // Base items
        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1", InProgress, ProjectTag);
        T t2(2, 1, "t2", "t1", "t2");

        // Create the source structure once and for all
        ModelStructure sourceStructure;
        sourceStructure << c1
                        << _+t1
                        << _+t2;

        ModelPath itemToRemove = c1 % t1;

        ModelPath itemToTest = c1 % t2;

        QTest::newRow( "remove empty collection" ) << sourceStructure
                                                   << itemToTest
                                                   << itemToRemove;
    }

    void shouldReactToModelReset_data()
    {
        QTest::addColumn<ModelStructure>( "sourceStructure" );
        QTest::addColumn<ModelStructure>( "outputStructure" );

        // Base items
        C c1(1, 0, "c1");
        T t1(1, 1, "t1", QString(), "t1", InProgress, ProjectTag);
        T t2(2, 1, "t2", "t1", "t2");

        // Create the source structure once and for all
        ModelStructure sourceStructure;
        sourceStructure << c1
                        << _+t1
                        << _+t2;

        ModelStructure outputStructure;

        QTest::newRow( "clear model" ) << sourceStructure
                                       << outputStructure;
    }


    void shouldReactToModelReset()
    {
        //GIVEN
        QFETCH(ModelStructure, sourceStructure);

        //Source model
        QStandardItemModel source;
        StandardModelBuilderBehavior behavior;
        behavior.setMetadataCreationEnabled(false);
        ModelUtils::create(&source, sourceStructure, ModelPath(), &behavior);

        //create metadataModel
        TodoMetadataModel metadataModel;
        ModelTest t1(&metadataModel);

        metadataModel.setSourceModel(&source);

        //WHEN
        source.clear();

        //THEN
        QStandardItemModel output;
        QCOMPARE(metadataModel, output);
    }
};

QTEST_KDEMAIN(TodoMetadataModelTest, GUI)

#include "todometadatamodeltest.moc"
