/***********************************************************************
 * This file is part of Scanned Image Extract.
 *
 * Scanned Image Extract is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scanned Image Extract is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scanned Image Extract.  If not, see <http://www.gnu.org/licenses/>
 *
 *
 * Copyright (C) 2015, Dominik Rueß; info@dominik-ruess.de
 **********************************************************************/

#include "imageboundary.h"

#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QPainter>
#include <QTimer>

#include "TargetImage.h"

ImageBoundary::ImageBoundary(QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
    , _isCopied(false)
    , _userHasSeenThis(false)
    , _boundaryPercentage(-1.0)
{

    QPointF corner[] = {QPointF(-100,-75),
                        QPointF(100,-75),
                        QPointF(100,75),
                        QPointF(-100,75)};
    setCorners(corner);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setBrush(NON_SELECTED);

}

QRectF ImageBoundary::boundingRect()
{
    QRectF out;
    QPointF mi(1e10,1e10), ma(-1e10, -1e10);
    for (int i=0; i<4; i++)
    {
        mi.setX(qMin(mi.x(), corners()[i].x()));
        mi.setY(qMin(mi.y(), corners()[i].y()));
        ma.setX(qMax(ma.x(), corners()[i].x()));
        ma.setY(qMax(ma.y(), corners()[i].y()));
    }
    out = QRectF(mi, ma);
    return out;
}

void ImageBoundary::setCorners(const QPointF corner[])
{
    QPainterPath path;

    path.moveTo(corner[0]);
    for (int i=0; i<4; i++) {
        _corners[i] = corner[i];
        if (i>0) {
            path.lineTo(corner[i]);
        }
    }
    path.lineTo(corner[0]);

    setPath(path);

}

const QPointF* ImageBoundary::corners() const
{
    return _corners;
}

void ImageBoundary::setCopied(const bool copied)
{
    dirty();
    _isCopied = copied;
}

void ImageBoundary::paint ( QPainter * painter,
             const QStyleOptionGraphicsItem * option,
             QWidget * widget  )
{
    QPen pen;
    if ( this->isSelected()) {
        setBrush(SELECTED);
        pen.setStyle(Qt::DashLine);
        QTime t = QTime::currentTime();
        QVector<qreal> dashes;
        dashes << 5 << 5;
        pen.setDashPattern(dashes);
        pen.setDashOffset(5*((float)t.second() + (float)t.msec()/1000.0));
    } else {
        setBrush(_isCopied ? COPIED : NON_SELECTED);
    }
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    QStyleOptionGraphicsItem option2 (*option);
    option2.state = option2.state & (~QStyle::State_Selected);
    QGraphicsPathItem::paint(painter, &option2, widget);
}

