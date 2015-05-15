/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "IntervalTreeView.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "RideFile.h"
#include "Context.h"
#include "Settings.h"
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>


IntervalTreeView::IntervalTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
#endif
    setStyleSheet("QTreeView::item:hover { background: lightGray; }");
    setMouseTracking(true);
    invisibleRootItem()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*,int)), this, SLOT(mouseHover(QTreeWidgetItem*,int)));
}

void
IntervalTreeView::mouseHover(QTreeWidgetItem *item, int)
{
    QVariant v = item->data(0, Qt::UserRole);
    IntervalItem *hover = static_cast<IntervalItem*>(v.value<void*>());

    // NULL is a tree, non-NULL is a node
    if (hover) context->notifyIntervalHover(hover);
}

void
IntervalTreeView::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* item1 = (QTreeWidgetItem *)itemAt(event->pos());
    QTreeWidget::dropEvent(event);
    QTreeWidgetItem* item2 = (QTreeWidgetItem *)itemAt(event->pos());

    if (item1==topLevelItem(0) || item1 != item2)
        QTreeWidget::itemChanged(item2, 0);
}

QStringList 
IntervalTreeView::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-intervals";

    return returning;
}

QMimeData *
IntervalTreeView::mimeData (const QList<QTreeWidgetItem *> items) const
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data 
    stream << (quint64)(context); // where did this come from?
    stream << (int)items.count();
    foreach (QTreeWidgetItem *p, items) {

        // convert to one of ours
        QVariant v = p->data(0, Qt::UserRole);
        IntervalItem *interval = static_cast<IntervalItem*>(v.value<void*>());

        // drag and drop tree !?
        if (interval == NULL) return returning;

        RideItem *ride = interval->rideItem();

        // serialize
        stream << interval->name; // name
        stream << (quint64)(ride);
        stream << (quint64)interval->start << (quint64)interval->stop; // start and stop in secs
        stream << (quint64)interval->startKM << (quint64)interval->stopKM; // start and stop km
        stream << (quint64)interval->displaySequence;

    }

    // and return as mime data
    returning->setData("application/x-gc-intervals", rawData);
    return returning;
}
