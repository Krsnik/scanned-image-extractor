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

#include "copytargets.h"

#include <opencv/cv.h>
#ifndef OPENCV2
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#else
#include <opencv/highgui.h>
#endif

#include <QDir>
#include <QDebug>

#include "extracttargets.h"

QMutex CopyTargets::memorySaveMutex;

CopyTargets::CopyTargets(QObject *parent)
  : QThread(parent)
  , _stopped(false)
  , _exitWritingOperationDone(false)
{
}

CopyTargets::~CopyTargets()
{
    _fileListMutex.lock();
    abortSaveFile(0);
    while (!_exitWritingOperationDone || _filesToCopy.size() > 0)
    {
        _fileListMutex.unlock();
        // saving may occur on closing
        qDebug() << "waiting for all writing operations to finish (still" << _filesToCopy.size() << "to write)";
        //_computationCondition.wait(&_computationRunning);
        //qDebug() << "finished writing next image";
        QThread::wait(500);
        _fileListMutex.lock();
    }
    _filesToCopy.clear();
    _fileListMutex.unlock();
    //_computationRunning.unlock();
    _stopped = true;
    _condition.wakeAll();

    wait();
}

void CopyTargets::addSaveFiles(SourceFilePtr source)
{
    QMutexLocker l(&_fileListMutex);
    _filesToCopy.push_back(source);
    _abortSaveFile = 0;

    if (!isRunning()) {
        start(QThread::LowPriority);
    } else {
        _condition.wakeOne();
    }

}

void CopyTargets::abortSaveFile(SourceFilePtr file)
{
    _abortSaveFile = file;
}

void CopyTargets::clear()
{
    QMutexLocker l(&_fileListMutex);
    _filesToCopy.clear();
}

#define ABORT_CURRENT_TEST(breakit) \
    if (_abortSaveFile.get() != 0 \
        && file->source.absoluteFilePath() == _abortSaveFile->source.absoluteFilePath()) \
    { \
        if (breakit) break; \
        else continue; \
    }

void CopyTargets::run()
{
    while (!_stopped) {
        SourceFilePtr file;
        {
            QMutexLocker l(&_fileListMutex);
            if (_filesToCopy.size() == 0) {
                _condition.wait(&_fileListMutex);
                // continue: in the meantime it might have been stopped
                // or set empty
                continue;
            }
            file= _filesToCopy.first();
            _filesToCopy.pop_front();
        }
        //QMutexLocker l(&_computationRunning);

        QMutexLocker l1(&ExtractTargets::imageMutex);
        QList<TargetImagePtr> targets = file->targets;
        QList<BackMapPtr> backmaps;
        for (int i=0; i<targets.size(); i++)
        {
            backmaps.append(targets[i]->backmap);
        }
        l1.unlock();

        QMutexLocker l2(&memorySaveMutex);
        cv::Mat src = cv::imread(file->source.canonicalFilePath().toLocal8Bit().data(),
                                 CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        QDir targetDir (QDir::rootPath());
        targetDir.mkpath(QFileInfo(_targetDir).absolutePath());

        for (int i=0; i<targets.size(); i++)
        {
            ABORT_CURRENT_TEST(true);

            if (targets[i]->boundary->getCopied()) {
                qDebug() << "Target " << i
                         << " of source "
                         << file->source.canonicalFilePath()
                         << " has already been copied in this session, skipping";
                continue;
            }
            if (targets[i]->copyId == -1)
            {
                targets[i]->copyId = ++TargetImage::nextCopyId;
            }
            const int tW = targets[i]->width;
            const int tH = targets[i]->height;
            if (tW == 0 || tH == 0) {
                continue;
            }

            cv::Mat mapX(tH, tW, CV_32FC1),
                    mapY(tH, tW, CV_32FC1);
            cv::Mat out(tH, tW, src.type());
            for (int r=0; r<tH; r++) {
                float* mX = mapX.ptr<float>(r);
                float* mY = mapY.ptr<float>(r);
                for (int c=0; c<tW; c++) {
                    backmaps[i]->transform(QPointF(c, r), mX[c], mY[c]);
                }
                ABORT_CURRENT_TEST(true);
            }
            ABORT_CURRENT_TEST(true);
            cv::transpose(mapX, mapX);
            cv::transpose(mapY, mapY);
            ABORT_CURRENT_TEST(false);

            cv::remap(src, out, mapX, mapY, CV_INTER_CUBIC);
            mapX.release();
            mapY.release();
            cv::flip(out, out, 0);
            QString newFileName(QString("%1%2%3_%4.%5")
                                .arg(_targetDir + QDir::separator())
                                .arg(_prefix)
                                .arg(file->source.baseName())
                                .arg(targets[i]->copyId, (int)FIELD_WIDTH, (int)10, QChar('0'))
                                .arg(file->source.suffix()));

            cv::Mat out2;
            int rot = targets[i]->determinedRotation + targets[i]->rotation + 1;

            if (out.size().width ==0 || out.size().height == 0)
            {
                emit copyError(file, targets[i]);
            }
            else
            {
                switch(rot) {
                case 1:
                case 5:
                case -3:
                    cv::transpose(out, out2);
                    cv::flip(out2, out2, 1);
                    break;
                case 2:
                case 6:
                case -2:
                    cv::flip(out, out2, -1);
                    break;
                case 3:
                case -1:
                    cv::transpose(out, out2);
                    cv::flip(out2, out2, 0);
                    break;
                default:
                    out2=out;
                    break;
                }
                if (cv::imwrite(newFileName.toLocal8Bit().data(), out2)) {
                    ABORT_CURRENT_TEST(true);
                    targets[i]->boundary->setCopied(true);
                    if (!_stopped)
                    {
                        emit copied(QFileInfo(newFileName).fileName(),
                                    QFileInfo(newFileName).absolutePath());
                    }
                } else {
                    if (!_stopped)
                    {
                        emit copyError(file, targets[i]);
                    }
                }
            }
        }
        ABORT_CURRENT_TEST(false);
        file->changed = false;
        src.release();
        l2.unlock();

        //l.unlock();

        _computationCondition.wakeAll();
    }
    _computationCondition.wakeAll();
}
