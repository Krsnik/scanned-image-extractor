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

#ifndef SOURCEFILE_H
#define SOURCEFILE_H

#include <QFileInfo>
#include <opencv/cv.h>

#include "TargetImage.h"


typedef std::tr1::shared_ptr<QGraphicsPixmapItem> QGraphicsPixmapItemPtr;

struct SourceFile
{
    QFileInfo source;
    QList<TargetImagePtr> targets;
    QGraphicsPixmapItemPtr image;
    QImage imageOrig;
    int id;

    bool error;
    bool changed;
    SourceFile()
        :  id(++nextId)
        , error(false)
        , changed(true)
    {
    }

    double scale;
private:
    static int nextId;

};

typedef std::shared_ptr<SourceFile> SourceFilePtr;

#endif // SOURCEFILE_H
