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

#ifndef COPYTARGETS_H
#define COPYTARGETS_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>

#include "sourcefile.h"

#define TARGET_FILENAME "scanner_extracted"
#define FIELD_WIDTH 2

class CopyTargets : public QThread
{
    Q_OBJECT
public:
    explicit CopyTargets(QObject *parent = 0);

    ~CopyTargets();

    void addSaveFiles(SourceFilePtr);

    void abortSaveFile(SourceFilePtr);

    void setDestination(const QString targetDir) { _targetDir = targetDir;}

    static QMutex memorySaveMutex;

    void clear();

    void setPrefix(QString prefix) { _prefix = prefix; }

    void setExitSaving() { _exitWritingOperationDone = true; }

signals:
    void copyError(SourceFilePtr source, TargetImagePtr target);
    void copied(QString filename, QString targetDir);

public slots:

protected:
    void run();

private:
    QVector<SourceFilePtr> _filesToCopy;

    QString _targetDir;

    QMutex _fileListMutex;
    // QMutex _computationRunning;
    QWaitCondition _condition;
    QWaitCondition _computationCondition;

    bool _stopped;

    QString _prefix;

    SourceFilePtr _abortSaveFile;

    bool _exitWritingOperationDone;
};

#endif // COPYTARGETS_H
