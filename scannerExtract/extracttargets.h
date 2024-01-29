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

#ifndef EXTRACTTARGETS_H
#define EXTRACTTARGETS_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QList>
#include <QPair>

#include "TargetImage.h"
#include "sourcefile.h"

class ExtractTargets : public QThread
{
    Q_OBJECT
public:
    explicit ExtractTargets(QObject *parent = 0);
    ~ExtractTargets();

    void addTarget(TargetImagePtr target,
                   SourceFilePtr source,
                   const bool highPriority = false);

    static QMutex imageMutex;
    static QMutex fileListMutex;

    QList<QPair<TargetImagePtr, SourceFilePtr> > getList() { return _targetList;  }

    void setCrop(const float crop) { _cropPercentage = crop; }



signals:
    void doneTarget(const TargetImagePtr&);

public slots:

    void stop();

protected:
    void run();

private:
    inline double _norm(const QPointF&);
    inline double _dot(const QPointF& p1, const QPointF& p2);
    inline double _norm2(const QPointF& p);

    QWaitCondition _condition;
    void extract(TargetImagePtr target, SourceFilePtr source);

    QList<QPair<TargetImagePtr, SourceFilePtr> > _targetList;

    bool _stopped;
    bool _abortCurrentStartNext;

    float _cropPercentage;

    TargetImagePtr _current;
};

#endif // EXTRACTTARGETS_H
