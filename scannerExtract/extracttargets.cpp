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

#include <QtCore/qmath.h>

#include "extracttargets.h"


QMutex ExtractTargets::imageMutex;
QMutex ExtractTargets::fileListMutex;

ExtractTargets::ExtractTargets(QObject *parent)
    : QThread(parent)
    , _stopped(false)
    , _abortCurrentStartNext(false)
{
}

void ExtractTargets::addTarget(TargetImagePtr target,
                               SourceFilePtr source,
                               const bool highPriority)
{
    QMutexLocker l(&fileListMutex);

    if (_current.get() != 0
            && _current->workOnId == target->workOnId)
    {
        // already loading -> do nothing
        return;
    }

    if (highPriority) {
        _abortCurrentStartNext = true;
    }

    QPair<TargetImagePtr, SourceFilePtr> item(target, source);
    if (highPriority) {
        _targetList.push_front(item);
    }
    else {
        _targetList.push_back(item);
    }

    // find duplicates and delete (work from back)
    for (int i=_targetList.size()-1; i>=0; i--) {
        for (int j=i-1; j>=0; j--) {
            if (_targetList[i].first->workOnId
                    == _targetList[j].first->workOnId) {
                _targetList.erase(_targetList.begin() + i);
            }
        }
    }

    _stopped = false;
    if (!isRunning()) {
        start(QThread::HighPriority);
    } else {
        _condition.wakeOne();
    }
}

void ExtractTargets::stop()
{
    _stopped = true;
}

ExtractTargets::~ExtractTargets()
{
    fileListMutex.lock();
    _targetList.clear();
    _stopped = true;
    _condition.wakeAll();
    fileListMutex.unlock();

    wait();
}

void ExtractTargets::run()
{
    while (!_stopped) {
        _abortCurrentStartNext = false;
        {
            QMutexLocker l(&fileListMutex);
            if (_targetList.size() == 0) {
                _condition.wait(&fileListMutex);
                // continue: in the meantime it might have been stopped
                // or set empty
                continue;
            }
        }

        fileListMutex.lock();
        _current = _targetList.first().first;
        SourceFilePtr currentSource = _targetList.first().second;
        fileListMutex.unlock();

        imageMutex.lock();
        extract(_current, currentSource);
        imageMutex.unlock();

        if (_stopped || _abortCurrentStartNext) {
            continue;
        }

        fileListMutex.lock();
        _targetList.pop_front();
        fileListMutex.unlock();

        if (!_stopped) {
            emit doneTarget(_current);
        }
        _current = TargetImagePtr();
    }
}

void ExtractTargets::extract(TargetImagePtr target, SourceFilePtr source)
{
    QImage out;
    if (source->imageOrig.isNull()
            || source->imageOrig.width() == 0
            || source->imageOrig.height() == 0)
    {
        target->backmap = BackMapPtr();
        target->image = QImage();
        return;
    }

    const QImage& sourceImage = source->imageOrig;

    const double cropPerc = target->boundary->getCrop() < 0 ? _cropPercentage : target->boundary->getCrop();
    target->boundary->setCrop(cropPerc);
    const int width = _norm(target->boundary->corners()[0] * (1-cropPerc)
                            -target->boundary->corners()[3] * (1-cropPerc));
    const int height = _norm(target->boundary->corners()[1] * (1-cropPerc)
                             -target->boundary->corners()[0] * (1-cropPerc));

    out = QImage(width, height, QImage::Format_RGB32);
    out.fill(QColor(0,0,0));

    // check correct order, such that it does not reflect in any dimension
    // (no flipping)

    double anglesSource[4];
    QPointF center(0,0);
    for (int i=0;i<4;i++) {
        center += target->boundary->corners()[0];
    }
    center /= 4;
    center = target->boundary->mapToScene(center);
    for (int i=0; i<4; i++) {
        QPointF curr = target->boundary->mapToScene(target->boundary->corners()[i])-center;
        anglesSource[i] = qAtan2(curr.y(), curr.x());
    }
    int numNeg = 0;
    for (int i=0; i<4; i++) {
        const double sgn1 = (anglesSource[i]-anglesSource[(i+3)%4]);
        //const double sgn2 = (anglesTarget[i]-anglesTarget[(i+3)%4]);
        if (sgn1 < 0) numNeg++;
    }

    // either double reflected (numNeg == 2) or only one side
    if (numNeg >= 2) {
        QPointF corners[4];
        if (numNeg > 2) {
            corners[0] = target->boundary->corners()[0];
            corners[1] = target->boundary->corners()[3];
            corners[2] = target->boundary->corners()[2];
            corners[3] = target->boundary->corners()[1];
        } else {
            corners[0] = target->boundary->corners()[2];
            corners[1] = target->boundary->corners()[3];
            corners[2] = target->boundary->corners()[0];
            corners[3] = target->boundary->corners()[1];

        }
        target->boundary->setCorners(corners);
        // start over again
        extract(target, source);
    }
    else
    {
        // now also check the orientation, it has to be
        // similar to the source. meaning roughly upright -> upright
        QPointF currRot = target->boundary->mapToScene(target->boundary->corners()[2])
                -target->boundary->mapToScene(target->boundary->corners()[1]);
        const double resultAngle = qAtan2( currRot.y(),
                                           currRot.x() );

        const int num90DegreeBins = qRound((resultAngle)/(M_PI/2.0));

        QPointF dirX(target->boundary->corners()[3]* (1-cropPerc)
                -target->boundary->corners()[0]* (1-cropPerc)),
                dirY(target->boundary->corners()[0]* (1-cropPerc)
                -target->boundary->corners()[1]* (1-cropPerc));        
        QSizeF size(_norm(dirX), _norm(dirY)) ;
        dirX /= size.width();
        dirY /= size.height();
        QPointF center(0.5*(target->boundary->mapToScene(target->boundary->corners()[0])
                       + target->boundary->mapToScene(target->boundary->corners()[2])));

        const int sourceHeight = sourceImage.height();
        const int sourceWidth = sourceImage.width();

        target->backmap = BackMapPtr(new BackMap(resultAngle,
                                                 center,
                                                 size,
                                                 source->scale));

#pragma omp parallel for
        for (int y=0; y<height; y++) {
            if (!_stopped && !_abortCurrentStartNext)
            {
                for (int x=0; x<width; x++) {
                    //QPointF pos(y -height/2.0, x -width/2.0);
                    QPointF pos(target->boundary->corners()[1]* (1-cropPerc)
                            + y*dirY + x*dirX);
                    pos = target->boundary->mapToScene(pos);
                    const int xSource = qRound(pos.x());
                    const int ySource = qRound(pos.y());
                    if (xSource >= 0
                            && xSource < sourceWidth
                            && ySource >= 0
                            && ySource < sourceHeight) {
                        out.setPixel(x,y, sourceImage.pixel(xSource, ySource));
                    } else {
                        out.setPixel(x,y,qRgb(0,0,0));
                    }
                }
            }
        }

        if (_stopped || _abortCurrentStartNext) {
            target->width = 0;
            target->height = 0;
            target->backmap = BackMapPtr();
            target->image = QImage();
            return;
        }
        else
        {
            target->width = (double)width * source->scale;
            target->height = (double)height * source->scale;
        }


        // finally, add normalizing rotation and user rotation:
        out = out.transformed(QTransform().rotate( num90DegreeBins*90).rotate(target->rotation*90));

        target->determinedRotation = num90DegreeBins;
        target->image = out;
    }
}

inline double ExtractTargets::_norm(const QPointF& p)
{
    return qSqrt(p.x()*p.x() + p.y()*p.y());
}


inline double ExtractTargets::_dot(const QPointF& p1, const QPointF& p2)
{
    return p1.x() * p2.x() + p1.y() * p2.y();
}

inline double ExtractTargets::_norm2(const QPointF& p)
{
    return p.x()*p.x() + p.y()*p.y();
}
