/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
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

#ifndef PIMITEMMONITOR_H
#define PIMITEMMONITOR_H
#include <QObject>
#include "akonadibaseitem.h"

namespace Akonadi {
class Monitor;
class Session;
}

class PimItemMonitor: public QObject
{
    Q_OBJECT
public:
    explicit PimItemMonitor(const AkonadiBaseItem::Ptr &item, QObject* parent = 0);
    
    enum ChangedPart {
        Title = 0x01,
        Text = 0x02,
        CreationDate = 0x04,
        LastModifiedDate = 0x08,
        Payload = 0x10, //The payload of the akonadi item has changed in any way
        AllParts = Title|Text|CreationDate|LastModifiedDate|Payload
    };
    Q_DECLARE_FLAGS(ChangedParts, ChangedPart)
    
    /**
     * store the item
     *
     * This does not emit changed() for this PimItem,
     * but if there are other PimItem refering to the same akonadi item,
     * changed will be emitted there
     *
     * If subsequent writes are needed, the monitor keeps our copy up to date, otherwise there would be conflicts.
     */
    bool saveItem();
    
signals:
    /**
     * emitted as soon as the item is ready to be accessed.
     */
	void payloadFetchComplete();
    /**
     * emitted if the akonadi item was changed from somwhere else than this instance of PimItem
     */
    void changed(PimItemMonitor::ChangedParts);
    /**
     * emitted after item was removed from storage
     */
    void removed();

private slots:
    /**
     * this will fetch the payload if not already fetched,
     * and emit payloadFetchComplete on completion
     */
    void fetchPayload();
    void itemFetchDone(KJob *job);

    /**
     * Update our copy, after a an external modification (update local variables) and emti changed signal
     */
    void updateItem(const Akonadi::Item &, const QSet<QByteArray>&);
    /**
     * update item after akonadi item was modified from this instance (local variables are already up to date)
     */
    void modifyDone( KJob *job );
    void enableMonitor();
private:
    Q_DISABLE_COPY(PimItemMonitor);

    void itemUpdated(const Akonadi::Item &);
    
    Akonadi::Monitor *m_monitor;
    Akonadi::Session *m_session;
    bool m_itemOutdated;
    AkonadiBaseItem::Ptr mItem;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PimItemMonitor::ChangedParts)
Q_DECLARE_METATYPE(PimItemMonitor::ChangedParts)

#endif // PIMITEMMONITOR_H
