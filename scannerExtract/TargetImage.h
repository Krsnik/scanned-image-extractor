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

#ifndef TARGETIMAGE_H
#define TARGETIMAGE_H

#include "imageboundary.h"

#include <QPixmap>
#include <QDebug>
#include <QtCore/qmath.h>

#ifdef __GNUC__
#include <tr1/memory>
#endif
class BackMap
{
public:
    BackMap(const double angle,
            const QPointF translation,
            const QSizeF size,
            const double scale)
    {
        const double c = qCos(angle), s = qSin(angle);
        rotation[0] = c;
        rotation[1] = -s;
        rotation[2] = s;
        rotation[3] = c;
        this->translation[0] = translation.x();
        this->translation[1] = translation.y();
        this->size[0] = size.width();
        this->size[1] = size.height();
        this->scale = scale;
    }

    inline void transform(const QPointF& input, float& x, float& y)
    {
        QPointF preTrans(input.x() - 0.5*size[0]*scale,
                input.y() - 0.5*size[1]*scale);
        x = rotation[0] * preTrans.x() + rotation[1] * preTrans.y() + translation[0]*scale;
        y = rotation[2] * preTrans.x() + rotation[3] * preTrans.y() + translation[1]*scale;
    }

private:

    double scale;
    double size[2];
    double rotation[4];
    double translation[2];
};

typedef std::tr1::shared_ptr<BackMap> BackMapPtr;

enum Rotation90
{
    TargetRotation0 = 0,
    TargetRotation90 = 1,
    TargetRotation180 = 2,
    TargetRotation270 = 3
};


struct TargetImage
{
    ImageBoundaryPtr boundary;

    QImage image;

    Rotation90 rotation;
    int determinedRotation;

    TargetImage()
        : boundary(new ImageBoundary)
        , rotation(TargetRotation0)
        , width(0)
        , height(0)
        , copyId(-1)
        , workOnId(++nextId)
        , aspect(-1.0)
    {}

    TargetImage(ImageBoundaryPtr newB)
        : boundary(newB)
        , rotation(TargetRotation0)
        , width(0)
        , height(0)
        , copyId(-1)
        , workOnId(++nextId)
        , aspect(-1.0)
    {}

    ~TargetImage()
    {
        backmap = BackMapPtr();
        boundary = ImageBoundaryPtr();
    }

    BackMapPtr backmap;
    int width;
    int height;

    int copyId;
    long int workOnId;

    float aspect;

    static int nextCopyId;
private:
    static int nextId;
};

typedef std::shared_ptr<TargetImage> TargetImagePtr;

#endif // TARGETIMAGE_H
