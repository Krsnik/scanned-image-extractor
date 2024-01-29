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

#ifndef IMAGEBOUNDARY_H
#define IMAGEBOUNDARY_H

#include <QtWidgets/QGraphicsPathItem>
#include <QPointF>
#include <QTimer>
#include <memory>
#include <QDebug>

#include <opencv/cv.h>

class TargetImage;

#define SELECTED QBrush(QColor(255,0,0,50))
#define NON_SELECTED QBrush(QColor(0,0,255,50))
#define COPIED QBrush(QColor(0,255,0,50))

class ImageBoundary : public QGraphicsPathItem
{

public:
    explicit ImageBoundary(QGraphicsItem *parent = 0);

    QRectF boundingRect();

    void paint ( QPainter * painter,
                 const QStyleOptionGraphicsItem * option,
                 QWidget * widget = 0 );

    void setCorners(const QPointF corner[]);

    const QPointF* corners() const;

    void setCopied(const bool copied);

    bool getCopied() const { return _isCopied; }

    bool getUserHasSeenThis() const { return _userHasSeenThis; }
    void setUserHasSeenThis() { _userHasSeenThis = true; }

    void setCrop(const double perc) { _boundaryPercentage = perc; }
    double getCrop() const { return _boundaryPercentage; }

    void dirty() { _isCopied = false; }

signals:

public slots:

private:
    int _lastSecond;

    QPointF _corners[4];

    bool _isCopied;

    bool _userHasSeenThis; // this allows to adapt to the current aspect ratio

    double _boundaryPercentage;

};

typedef std::shared_ptr<ImageBoundary> ImageBoundaryPtr;

#endif // IMAGEBOUNDARY_H
