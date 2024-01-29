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

#include "imagescene.h"

#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QGraphicsView>
#include <QTransform>
#include <QKeyEvent>
#include <QScrollBar>
#include <QtCore/qmath.h>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QStatusBar>

#include "settings.h"
// #define DEBUG_TEST

ImageScene::ImageScene(QObject *parent)
    : QGraphicsScene(parent)
    , _currentState(MouseStateNotLoaded)
    , _currentMouseState(MouseStateNone)
    , _sizeChange(":/images/sizeChange.png")
    , _rotate(":/images/rotate.png")
    , _currentIndex(-1)
    , _currLine(0)
    , _cropPercentage(0)
    , _aspectRatio(-1)
    , _backgroundLoader(0)
    , _isWaiting(false)
    , _enforceAspectForNew(false)
    , _waitBar(0)
    , _numNeighbours(2)
{
    qRegisterMetaType<QFileInfoList>("QFileInfoList");
    qRegisterMetaType<SourceFilePtr>("SourceFilePtr");
    qRegisterMetaType<TargetImagePtr>("TargetImagePtr");
    connect(this, SIGNAL(selectionChanged()),
            this, SLOT(onSelectionChanged()), Qt::QueuedConnection);

    connect(&_copyTargetThread, SIGNAL(copied(QString,QString)),
            SLOT(doneCopy(QString,QString)), Qt::QueuedConnection);
    connect(&_copyTargetThread, SIGNAL(copied(QString,QString)),
            SIGNAL(doneCopying(QString,QString)), Qt::QueuedConnection);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(updateSelection()), Qt::QueuedConnection);


    connect(this, SIGNAL(renewList(QFileInfoList,int)),
            this, SLOT(newImageList(QFileInfoList,int)), Qt::QueuedConnection);

    /*
    connect(this, SIGNAL(extractTarget(int,int)), SLOT(_extractTarget(int,int)),
            Qt::QueuedConnection);*/

    connect(&_targetExtractor, SIGNAL(doneTarget(const TargetImagePtr&)),
            this, SLOT(doneLoadingTarget(const TargetImagePtr&)), Qt::BlockingQueuedConnection);

    connect(&_copyTargetThread, SIGNAL(copyError(SourceFilePtr,TargetImagePtr)),
            this, SLOT(_onCopyError(SourceFilePtr,TargetImagePtr)));
    _timer.setInterval(250);
    _timer.start();

}

ImageScene::~ImageScene()
{
    if (_backgroundLoader != 0)
    {
        delete _backgroundLoader;
    }
    disconnect(this);
    saveCurrent();
    _copyTargetThread.setExitSaving();
    delete _currLine;
}


void ImageScene::updateSelection()
{
    // for a selection animation
    if (_currentIndex >= 0
            && _currentIndex < _currentFiles.size()
            && _currentFiles[_currentIndex]->targets.size() > 0)
    {
        for (int i=0; i<_currentFiles[_currentIndex]->targets.size(); i++)
        {
            _currentFiles[_currentIndex]->targets[i]->boundary->update();
        }
    }
    update();
}

void ImageScene::init()
{


    QListIterator<QGraphicsItem *> i(items());
    while (i.hasNext())
    {
        QGraphicsItem* item = i.next();
        item->setVisible(false);
        removeItem(item); // does not perform "delete" (would destroy use of smartptr)
    }

    QList<QGraphicsView *> v = views();
    _view = v.size() > 0 ? v[0] : NULL;
    _view->setCursor(Qt::CrossCursor);
    _view->viewport()->setMouseTracking(true);

    update();
    _view->resetMatrix();
    _view->resetTransform();
    _view->update();

}


void ImageScene::newImageList(const QFileInfoList& images,
                              const int selectedIndex)
{

    if (!_extractMutex.tryLock())
    {
        emit (renewList(images, selectedIndex));
    }

    _copyTargetThread.clear();
    if (_backgroundLoader != 0) {
        disconnect(_backgroundLoader);
        _backgroundLoader->setPriority(QThread::LowPriority);
        _backgroundLoader->disconnect();
        _backgroundLoader->stop();
        _backgroundLoader->clear();
        _backgroundLoader->deleteLater();
    }

    _backgroundLoader = new PreloadSource();

    connect(_backgroundLoader, SIGNAL(doneLoading(SourceFilePtr,int)),
            this, SLOT(_doneLoading(SourceFilePtr,int)), Qt::QueuedConnection);
    _currentFiles.clear();
    for (int i=0; i<images.size(); i++) {
        SourceFilePtr tmp (new SourceFile);
        tmp->source = images[i];
        _currentFiles.push_back(tmp);
    }
    _currentIndex = qMax(0,selectedIndex);
    _currentTarget = 0;


    _extractMutex.unlock();
    emit reloadSettings(); // update settings for thread

    _isWaiting = false;
    loadPosition(_currentIndex, false, true);
}

void ImageScene::saveCurrent(const bool noUpdate, const bool force)
{
    if (_currentIndex>=0
            && _currentIndex < _currentFiles.size()
            && _currentFiles[_currentIndex]->targets.size() > 0) {
        clearSelection();
        if (!noUpdate)
        {
            emit updateTargetDisplay(QPixmap());
        }
        if (force)
        {
            for (int i=0; i<_currentFiles[_currentIndex]->targets.size(); i++)
            {
                _currentFiles[_currentIndex]->targets[i]->boundary->setCopied(false);
            }
        }
        _copyTargetThread.addSaveFiles(_currentFiles[_currentIndex]);
    }
}

void ImageScene::loadPosition(const int newPosition,
                              const bool increment,
                              const bool forceReload)
{
    bool positionChanged = false;
    if ( (newPosition >= 0 && !increment)
         || (newPosition == -2 && !increment)
         || (increment)) {
        const int newPos = newPosition == -2 ? _currentFiles.size() -1 : (increment ? _currentIndex + newPosition : newPosition);
        if (newPos>= 0 && newPos <_currentFiles.size()) {
            positionChanged = _currentIndex != newPos;

            if (positionChanged) {
                _copyTargetThread.abortSaveFile(0);
                saveCurrent();
            }

            _currentIndex = newPos;
        }
    }

    if (_currentIndex <0 ||  _currentIndex >= _currentFiles.size())
    {
        return;
    }

    if (_currentIndex  >=0 && _currentIndex < _currentFiles.size())
    {
        emit changed(_currentFiles[_currentIndex]->changed);
        emit fileName(_currentFiles[_currentIndex]->source.fileName());
        emit filePosition(QString(tr("%1 of %2")).arg(_currentIndex + 1).arg(_currentFiles.size()));
    }
#ifdef DEBUG_TEST
    // work on all images in selected directory
    static bool done = false;
    if (!done) {
        qDebug() << "changed";
        const int m = 3;
        QVector<float> w(m);
        w[0] = 0.0; w[1] = 1; w[2] = 5; //3.0; w[2] = 7.0; w[3] = 15;
     /*   for (int i=0; i<m; i++) {
            for (int j=0; j<m; j++) {
                for (int k=0; k<m; k++) {
                    for (int l=0; l<m; l++) {
                        cv::Vec4i values(w[i], w[j], w[k], w[l] );
                        if (w[i] == 0 && w[j] == 0 && w[k] == 0 && w[l] == 0) continue;
                        qDebug() << w[i] << w[j] << w[k] << w[l];*/
        cv::Vec4f values(0.1,5,1,10);
                        for (int a=0; a<_currentFiles.size(); a++)
                        {
                            //SourceFilePtr file(new SourceFile);
                            //file->source = ->source.absoluteFilePath();
                            _backgroundLoader->addLoadFiles(
                                        _currentFiles[a],
                                    m,
                                    values,
                                    false,
                                    false);
                        }
                    /*}
                }
            }
        }*/
        done = true;
        qDebug() << "done";
        return;
    }
#endif

    if ( (_isWaiting && !positionChanged)
         || _currentFiles[_currentIndex]->error)
    {
        if (_currentFiles[_currentIndex]->error)
        {
            emit fileName(_currentFiles[_currentIndex]->source.fileName()
                          + " - <font style=\"color:red;\">ERROR LOADING SOURCE</font>");
            _copyTargetThread.abortSaveFile(0);
            if (_waitBar != 0)
            {
                _waitBar->setVisible(false);
                delete _waitBar;
                _waitBar = 0;

                _isWaiting = false;
            }
            clear();
            emit updateTargetDisplay(QPixmap());
            return;
       }
       if (_currentFiles[_currentIndex]->imageOrig.isNull()) {
           QTimer::singleShot(500, this, SLOT(loadPosition()));
           return;
       }
    }

    // do clear target display
    emit setTargetWaiting(false);
    emit updateTargetDisplay(QPixmap());

    bool alreadyLoaded= false, isLoading = false;
    if (positionChanged) {

        if (!_currentFiles[_currentIndex]->imageOrig.isNull()) {
            alreadyLoaded = true;
        }
        if (_backgroundLoader->isCurrentlyLoading() ==
                _currentFiles[_currentIndex]->source.absoluteFilePath()) {
            isLoading = true;
        }
    }

    if (isLoading) {
        if (_waitBar == 0 || !_waitBar->isVisible())
        {
            _waitBar = new QProgressBar(_view);
            _waitBar->setWindowModality(Qt::ApplicationModal);
            _waitBar->setMaximum(0);
            _waitBar->setMinimum(0);
            _waitBar->show();
        }
        init(); // clear display
        return;
    }

    if ((!alreadyLoaded && positionChanged)
            || forceReload
            || _currentFiles[_currentIndex]->image.get() == NULL) {
        emit fileName(_currentFiles[_currentIndex]->source.fileName());
        init(); // clear display

        bool isSaved = !_currentFiles[_currentIndex]->changed;
        _backgroundLoader->addLoadFiles(_currentFiles[_currentIndex],
                                        _currentIndex,
                                        true,
                                        isSaved);

        if (_waitBar == 0 || !_waitBar->isVisible())
        {
            _waitBar = new QProgressBar(_view);
            _waitBar->setWindowModality(Qt::ApplicationModal);
            _waitBar->setMaximum(0);
            _waitBar->setMinimum(0);
            _waitBar->show();
        }
        _isWaiting = true;
        return; // done setting up threads
    }


    // start preloading for neighbours, start and end

    for (int i=_numNeighbours-1; i>=0; i--)
    {
        for (int j=0; j<2; j++)
        {
            const int ind = j==1 ?
                        qMax(0, _currentIndex - i -1)
                      : qMin(_currentFiles.size()-1, _currentIndex + i + 1);
            if (_currentFiles[ind]->image.get() == NULL)
            {                
                bool isSaved = !_currentFiles[ind]->changed;
                _backgroundLoader->addLoadFiles(_currentFiles[ind],
                                                ind,
                                                false,
                                                isSaved);                
            }
        }
    }

    // update current display
    clear();
    _currentFiles[_currentIndex]->image->setVisible(true);
    _currentFiles[_currentIndex]->image->setZValue(-100);
    addItem(_currentFiles[_currentIndex]->image.get());
    _view->resetMatrix();
    _view->fitInView(_currentFiles[_currentIndex]->image.get(),
                     Qt::KeepAspectRatio);

    for (int i=0; i<_currentFiles[_currentIndex]->targets.size(); i++) {
        _currentFiles[_currentIndex]->targets[i]->boundary->setVisible(true);
        _currentFiles[_currentIndex]->targets[i]->boundary->setZValue(10+i);
        addItem(_currentFiles[_currentIndex]->targets[i]->boundary.get());
        if (!_currentFiles[_currentIndex]->targets[i]->boundary->getUserHasSeenThis()
                && _enforceAspectForNew)
        {
            _correctAspectRatio(_currentFiles[_currentIndex]->targets[i]);
        }
        _currentFiles[_currentIndex]->targets[i]->boundary->setUserHasSeenThis();
        qApp->processEvents();
    }

    if (_currentIndex >=0)
    {
        // select one item - but only if not all of them have been copied, already
        if (_currentFiles[_currentIndex]->targets.size() > 0) {
            int ind=-1;
            for (int i=_currentFiles[_currentIndex]->targets.size()-1; i>=0; i--)
            {
                if (!_currentFiles[_currentIndex]->targets[i]->boundary->getCopied()) {
                    ind = i;
                }
            }
            clearSelection();
            if (ind >=0) {
                _currentFiles[_currentIndex]->targets[ind]->boundary->setSelected(true);
                emit targetPosition(QString(tr("%1 of %2")).arg(ind + 1).arg(_currentFiles[_currentIndex]->targets.size()));
            }
        }

        emit changed(_currentFiles[_currentIndex]->changed);
        emit fileName(_currentFiles[_currentIndex]->source.fileName());
        emit filePosition(QString(tr("%1 of %2")).arg(_currentIndex + 1).arg(_currentFiles.size()));
    }

    if (_waitBar != 0) {
        _waitBar->setVisible(false);
        delete _waitBar;
        _waitBar = 0;
    }

    _currentState = MouseStateNone;
    _currentMouseState = MouseStateNone;
    _isWaiting = false;
    _view->update();
    update();

    _copyTargetThread.abortSaveFile(_currentFiles[_currentIndex]);

    // free old data - but only, if it has been saved, already
    // the target extraction may still work in one of the old images
    // so lock

    while (!ExtractTargets::fileListMutex.tryLock(50))
    {
        qApp->processEvents();
    }
    for (int i=0; i<_currentFiles.size(); i++) {
        if (i == _currentIndex) continue;

        bool inWaitingList = false;
        QListIterator<QPair<TargetImagePtr, SourceFilePtr>> it(_targetExtractor.getList());
        while (it.hasNext())
        {
            if (it.next().second->id == _currentFiles[i]->id)
            {
                inWaitingList = true;
                break;
            }
        }

        bool tooFar = qAbs(i-_currentIndex) > _numNeighbours
                && (!_currentFiles[i]->changed
                    || _currentFiles[i]->error
                    || (_currentFiles[i]->targets.size() > 0
                        && !_currentFiles[i]->targets[0]->boundary->getUserHasSeenThis()));
        if (tooFar && !inWaitingList) {
            _currentFiles[i]->image = QGraphicsPixmapItemPtr();
            _currentFiles[i]->imageOrig = QImage();
            for (int j=0; j<_currentFiles[i]->targets.size(); j++) {
                _currentFiles[i]->targets[j]->image = QImage();
                _currentFiles[i]->targets[j]->backmap = BackMapPtr();
            }
        }
    }
    ExtractTargets::fileListMutex.unlock();
}

void ImageScene::_doneLoading(const SourceFilePtr &source, const int position)
{
#ifdef DEBUG_TEST
    return;
#endif
    if (position>=0 && position <_currentFiles.size())
    {
        if ( _currentFiles[position]->id == source->id)
        {
            _currentFiles[position]->image = QGraphicsPixmapItemPtr(new QGraphicsPixmapItem(
                                                                        QPixmap::fromImage(_currentFiles[position]->imageOrig)
                                                                        )
                                                                    );
            int currentId = -1;
            for (int i=_currentFiles[position]->targets.size()-1; i>=0; i--)
            {
                _currentFiles[position]->targets[i]->boundary->setCrop(_cropPercentage);
                if (_enforceAspectForNew)
                {
                    _currentFiles[position]->targets[i]->aspect = _aspectRatio;
                    _correctAspectRatio(_currentFiles[position]->targets[i]);
                }
                if (position == _currentIndex && i == 0)
                {
                    currentId = i;
                }
                else
                {
                    _extractTarget(position, _currentFiles[position]->targets[i]);
                }
            }
            if (currentId >= 0)
            {
                // insert current lastly, since we want to have it first
                _currentTarget = _currentFiles[position]->targets[0];
                _extractTarget(position, _currentFiles[position]->targets[currentId], true);
            }
            if (position == _currentIndex)
            {
                loadPosition();
            }
        }
    }
}

TargetImagePtr ImageScene::addBoundary(ImageBoundaryPtr newB)
{
    _currentFiles[_currentIndex]->targets.push_back(TargetImagePtr(new TargetImage(newB)));
    addItem(newB.get());
    return _currentFiles[_currentIndex]->targets.last();
}

void ImageScene::onSelectionChanged()
{

    if (_currentState != MouseStateNewItem
            && _currentMouseState != MouseStateNewItem) {
        QList<TargetImagePtr> items = _findSelectedTarget();
        emit numSelected(items.size());
        double rotation =-1.0, aspect = -1.0, crop = -1.0;
        bool uniqueRotation = true, uniqueAspect = true, uniqueCrop = true;

        if (items.size() > 0) {
            _currentTarget = items.first();
        }
        else
        {
            _currentTarget = TargetImagePtr();
        }

        QListIterator<TargetImagePtr> i (items);
        while (i.hasNext())
        {
            TargetImagePtr target = i.next();
            if (rotation < 0) {
                rotation = target->rotation;
                aspect = target->aspect;
                crop = target->boundary->getCrop();
            }

            if (rotation != target->rotation) {
                uniqueRotation = false;
            }
            if (aspect != target->aspect) {
                uniqueAspect = false;
            }
            if (crop != target->boundary->getCrop()) {
                uniqueCrop = false;
            }

            if (!uniqueRotation && !uniqueAspect && !uniqueCrop) {
                break;
            }
        }

        if (items.size() > 0)
        {
            if (uniqueRotation) {
                emit rotation90(items[0]->rotation);
            } else {
                emit noRotation();
            }
            //if (aspect >=1) {
                if (uniqueAspect) {
                    emit selectedAspect(items[0]->aspect);
                } else {
                    emit noAspect();
                }
            //}
            if (crop > 0) {
                if (uniqueCrop) {
                    emit newCrop(items[0]->boundary->getCrop());
                } else {
                    emit noCrop();
                }
            }
        }
        _currentMouseState = MouseStateMoveItem;
        _view->setCursor(Qt::OpenHandCursor);
        _updateTarget(false);
    }
    update();
    _view->update();
}

void ImageScene::_updateTarget(const bool targetChanged, bool dirty)
{

    // search for targets
    QListIterator<TargetImagePtr> it(_findSelectedTarget());
    while (it.hasNext())
    {
        TargetImagePtr currTarget = it.next();

        if (targetChanged
                || currTarget->image.width() == 0
                || currTarget->image.height() == 0
                || currTarget->image.isNull())
        {
            QListIterator<TargetImagePtr> i2(_currentFiles[_currentIndex]->targets);
            while (i2.hasNext())
            {
                if (i2.next()->workOnId == currTarget->workOnId)
                {
                    emit setTargetWaiting(true);
                    _extractTarget(_currentIndex, currTarget, true);
                    break;
                }
            }
        }
        else
        {
            QPixmap out;
            QImage img = currTarget->image;

            QListIterator<TargetImagePtr> i2(_currentFiles[_currentIndex]->targets);
            while (i2.hasNext())
            {
                if (i2.next()->workOnId == currTarget->workOnId)
                {
                    out = QPixmap::fromImage(img);
                    emit setTargetWaiting(false);
                    emit updateTargetDisplay(out);
                    break;
                }
            }
        }
        if (dirty)
        {
            currTarget->boundary->setCopied(false);
            currTarget->boundary->dirty();
        }
    }


    if (dirty) {
        _currentFiles[_currentIndex]->changed=true;
        emit changed(true);
    }
}

void ImageScene::_extractTarget(const int fromImage,
                                const TargetImagePtr target,
                                const bool highPriority)
{
    if (fromImage >=0 && fromImage < _currentFiles.size())
    {
        if (!target->boundary->getUserHasSeenThis()
                || target->boundary->getCrop()<0)
        {
            target->boundary->setCrop(_cropPercentage);
        }

/*        target->boundary->setFlags(
                    QGraphicsItem::ItemHasNoContents
                    | target->boundary->flags());*/
        _targetExtractor.addTarget(target,
                                   _currentFiles[fromImage],
                                   highPriority);

    }
}

void ImageScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const double numDegrees = event->delta()/8.0;
        const double numSteps = numDegrees / 15.0;
        double factor = qPow(1.12, numSteps);

        QPointF scenePos = _view->mapToScene(
                    _view->mapFromGlobal(event->screenPos())
                    );
        zoom(factor);
        _view->centerOn(scenePos);
        event->accept();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // scroll left-right
        const int numPixRight = event->delta();
        if (_view->horizontalScrollBar () != 0) {
            //_view->horizontalScrollBar()->scroll(numPixRight, 0);
            _view->horizontalScrollBar()->setValue(
                        _view->horizontalScrollBar()->value() - numPixRight);
        }
        event->accept();
    } else {
        event->ignore();
    }

    // otherwise the scrolling will be up-/down
}

void ImageScene::doneCopy(const QString &, const QString &)
{
    bool changes = false;
    if (_currentIndex >= 0 && _currentIndex <_currentFiles.size())
    {
        bool changes = _currentFiles[_currentIndex]->targets.size() > 0;
        for (int k=0; k<(int)_currentFiles[_currentIndex]->targets.size(); k++)
        {
            changes &= _currentFiles[_currentIndex]->targets[k]->boundary->getCopied();
        }
    }
    emit changed(!changes);
    this->update();
}

void ImageScene::keyPressEvent ( QKeyEvent * keyEvent )
{
    if (keyEvent->key() == Qt::Key_Plus) {
        zoom(1.25);

    } else if (keyEvent->key() == Qt::Key_Minus) {
        zoom(0.8);

    } else if (keyEvent->key() == Qt::Key_Delete
               || keyEvent->key() == Qt::Key_Backspace) {
        deleteSelection();

    } else if (keyEvent->key() == Qt::Key_Escape) {
        if (_currentState == MouseStateNewItem) {
            removeItem(_currLine);
            delete _currLine;
            _currLine = 0;
        }
        else if (_currentMouseState == MouseStateNewItem)
        {
            deleteSelection();
        }
        _currentState = MouseStateNone;
        _currentMouseState = MouseStateNone;
        _view->setCursor(Qt::CrossCursor);

    } /*else if (keyEvent->key() == Qt::Key_Tab) {
        bool changed=false;
        TargetImagePtr target = _findSelectedTarget()[0];
        if (target != 0) {
            QPointF corners[4];
            for (int i=0;i<4;i++) corners[i]= target->boundary->corners()[i];
            const double l1 = _norm(corners[1]-corners[0]);
            const double l2 = _norm(corners[2]-corners[1]);
            corners[0] *= l1/l2;
            corners[2] *= l1/l2;
            corners[1] *= l2/l1;
            corners[3] *= l2/l1;
            target->boundary->setCorners(corners);
            _updateTarget(true);
            changed = true;
        }
        if (!changed) {
            QGraphicsScene::keyPressEvent(keyEvent);
        }

    } */
    else {
        QGraphicsScene::keyPressEvent(keyEvent);
    }
}

QList<TargetImagePtr> ImageScene::_findSelectedTarget()
{
    QList<TargetImagePtr> out;

    if (selectedItems().size() == 0
            || _currentFiles[_currentIndex]->targets.size() ==0)
        return out;

    QMap<ImageBoundary*, TargetImagePtr> map;
    QListIterator<TargetImagePtr> iT(_currentFiles[_currentIndex]->targets);
    int ind=0;
    while (iT.hasNext())
    {
        TargetImagePtr t = iT.next();
        map.insert(t->boundary.get(), t);
        ind++;
    }

    QListIterator<QGraphicsItem*> i(selectedItems());
    while (i.hasNext())
    {
        ImageBoundary* item = dynamic_cast<ImageBoundary*>(i.next());
        if (item) {
            out.append(map[item]);
        }
    }

    return out;
}

void ImageScene::zoom(double factor)
{
    if (_view == NULL) {
        return;
    }

    _view->scale(factor, factor);
    if (_currLine != NULL) {
        const QPointF p1 = _view->mapToScene(
                    _view->mapFromGlobal(QPoint(0,0))
                    );
        const QPointF p2 = _view->mapToScene(
                    _view->mapFromGlobal(QPoint(0,1))
                    );
        const double width = 3.0 * _norm(p1-p2);
        QPen pen = _currLine->pen();
        pen.setWidth(width);
        _currLine->setPen(pen);
    }
}


inline double ImageScene::_norm(const QPointF& p)
{
    return qSqrt(p.x()*p.x() + p.y()*p.y());
}


inline double ImageScene::_dot(const QPointF& p1, const QPointF& p2)
{
    return p1.x() * p2.x() + p1.y() * p2.y();
}

inline double ImageScene::_norm2(const QPointF& p)
{
    return p.x()*p.x() + p.y()*p.y();
}

inline double ImageScene::_lineDist(const QPointF& l1,
                                   const QPointF& l2,
                                   const QPointF& p)
{
    return ( (l2.x() - l1.x())*(l1.y() - p.y())
                - (l1.x() - p.x())*(l2.y() - l1.y()))
            / (qSqrt( (l2.x() - l1.x() ) * (l2.x() - l1.x() )
                      + (l2.y() - l1.y())*(l2.y() - l1.y()) ));
}

void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (_currentState == MouseStateNotLoaded) {
        return;
    }

    if (mouseEvent->button() != Qt::LeftButton) {
        if (_currentState == MouseStateNewItem) {
            _currentState = MouseStateNone;
            _currentMouseState = MouseStateNone;
            removeItem(_currLine);
            delete _currLine;
            _currLine = 0;
            _view->setCursor(Qt::CrossCursor);
        }
        return;
    }

    if (_currentState == MouseStateNone
            && _currentMouseState == MouseStateReleaseItem)
    {
        // first corner
        _view->setCursor(Qt::CrossCursor);
        _currentMouseState = MouseStateNone;
        clearSelection();
    } else if (_currentState == MouseStateNone
            && _currentMouseState == MouseStateNone
            && selectedItems().size() == 0)
    {
        // draw line
        const QPointF currMousePoint = _view->mapToScene(
                        _view->mapFromGlobal(mouseEvent->screenPos())
                        );

        const QPointF p1 = _view->mapToScene(
                    _view->mapFromGlobal(QPoint(0,0))
                        );
        const QPointF p2 = _view->mapToScene(
                    _view->mapFromGlobal(QPoint(0,1))
                        );
        const double width = 3.0 * _norm(p1-p2);
        delete _currLine;
        _lastCornerPos[0] = currMousePoint;
        _currLine = new QGraphicsLineItem(QLineF(currMousePoint, currMousePoint));
        QPen pen;
        pen.setWidth(width);
        pen.setStyle(Qt::DashDotLine);
        pen.setColor(NEW_BOX_LINE_COLOR);
        _currLine->setPen(pen);
        addItem(_currLine);
        _currentState = MouseStateNewItem;
    } else if (_currentState == MouseStateMoveCorner
            && _currentMouseState == MouseStateNewItem)
    {
        // switch to release mode, such that the next release of
        // the mouse stops the process
        _currentMouseState = MouseStateNewItemRelease;
    } else if (_currentState == MouseStateNone
            && _currentMouseState == MouseStateMoveItem)
    {
        _view->setCursor(Qt::ClosedHandCursor);
        _currentState = MouseStateMoveItem;
        QGraphicsScene::mousePressEvent(mouseEvent);

    } else if (_currentState == MouseStateNone
               && _currentMouseState == MouseStateRotateItem)
    {
        _currentState = MouseStateRotateItem;
        _lastTransformation = _currentTarget->boundary->transform();

        _rotationCenter = QPointF(0,0);
        for (int i=0; i<4; i++) {
            QPointF posCorner = _view->mapToGlobal(_view->mapFromScene(
                    _currentTarget->boundary->mapToScene(_currentTarget->boundary->corners()[i])));
            _rotationCenter += posCorner;
        }
        _rotationCenter /= 4;
        QPointF lastAngleDir = mouseEvent->screenPos() - _rotationCenter;
        _lastAngle = qAtan2(lastAngleDir.x(), lastAngleDir.y());
        QGraphicsScene::mousePressEvent(mouseEvent);

    } else if (_currentState == MouseStateNone
               && _currentMouseState == MouseStateMoveCorner)
    {
        _currentState = MouseStateMoveCorner;
        for (int i=0; i<4; i++) {
            _lastCornerPos[i] = _currentTarget->boundary->corners()[i];
        }
        _lastTransformation = _currentTarget->boundary->transform();
    } else if (_currentState == MouseStateNone
               && _currentMouseState == MouseStateMoveEdge)
    {
        _currentState = MouseStateMoveEdge;
        for (int i=0; i<4; i++) {
            _lastCornerPos[i] = _currentTarget->boundary->corners()[i];
        }
        _lastTransformation = _currentTarget->boundary->transform();
        const double lCurr = _norm(_currentTarget->boundary->corners()[(_currentEdge+1)%4] - _currentTarget->boundary->corners()[_currentEdge]);
        const double lOrth = _norm(_currentTarget->boundary->corners()[(_currentEdge+3)%4]
                                   - _currentTarget->boundary->corners()[_currentEdge]);
        _currAspectRatio = ( lCurr > lOrth ? _aspectRatio : 1.0/_aspectRatio);
    } else {
        QGraphicsScene::mousePressEvent(mouseEvent);
    }
}


void ImageScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (_view == NULL) {
        return;
    }

    if (_currentState == MouseStateNone) {
        bool found = false;
        for (int k=0; k<(int)_currentFiles[_currentIndex]->targets.size() && !found; k++) {
            if (!_currentFiles[_currentIndex]->targets[k]->boundary->isSelected()) {
                continue;
            }

            // check if mouse above one corner
            for (int i=0; i<4; i++) {                ;
                QPointF posCorner = _view->mapToGlobal(_view->mapFromScene(
                        _currentFiles[_currentIndex]->targets[k]->boundary->mapToScene(
                                                               _currentFiles[_currentIndex]->targets[k]->boundary->corners()[i])));

                if (QPointF(mouseEvent->screenPos() - posCorner).manhattanLength() < DISTANCE_GRAB_CORNER
                        && ! (mouseEvent->modifiers() & Qt::ShiftModifier)) {
                    if (_currentMouseState != MouseStateMoveCorner) {
                        QPointF center = QPointF(0,0);
                        for (int j=0; j<4;j++) {
                            center += _currentFiles[_currentIndex]->targets[k]->boundary->corners()[j];
                        }
                        center /= 4.0;
                        center = _view->mapToGlobal(_view->mapFromScene(
                                                                  _currentFiles[_currentIndex]->targets[k]->boundary->mapToScene(center)));
                        QPointF corner = _view->mapToGlobal(_view->mapFromScene(
                                                                _currentFiles[_currentIndex]->targets[k]->boundary->mapToScene(
                                                                    _currentFiles[_currentIndex]->targets[k]->boundary->corners()[i]))) - center;
                        qreal angle = qAtan2(corner.y(), corner.x());
                        QCursor tmp = _sizeChange;
                        const QPixmap cursorPix = tmp.pixmap().transformed(QTransform().rotateRadians(angle ),Qt::SmoothTransformation);
                        tmp = QCursor(cursorPix);
                        _view->setCursor(tmp);
                        _currentMouseState = MouseStateMoveCorner;
                    }

                    _currentCorner = i;
                    _currentTarget = _currentFiles[_currentIndex]->targets[k];
                    found = true;
                    break;
                }
            }

            // check if mouse above edge
            if (!found && !(mouseEvent->modifiers() & Qt::ShiftModifier)) {
                for (int i=0; i<4; i++) {
                    const QPointF p1 =  _view->mapToGlobal(
                                _view->mapFromScene(
                                    _currentFiles[_currentIndex]->targets[k]->boundary->mapToScene(
                                        _currentFiles[_currentIndex]->targets[k]->boundary->corners()[i])));
                    const QPointF p2 =  _view->mapToGlobal(
                                _view->mapFromScene(
                                    _currentFiles[_currentIndex]->targets[k]->boundary->mapToScene(
                                        _currentFiles[_currentIndex]->targets[k]->boundary->corners()[(i+1)%4])));
                    const QPointF diff = p1  -p2;
                    const double dist2 = _norm2(diff);
                    QPointF mP = mouseEvent->screenPos();
                    if (_norm2(mP - p1) < dist2
                            && _norm2(mP - p2) < dist2)
                    {
                        // = is in between both points
                        // now compute distance to connecting line
                        if (qAbs(_lineDist(p1, p2, mP)) < DISTANCE_GRAB_EDGE) {
                            qreal angle = qAtan2(diff.y(), diff.x()) - M_PI/2.0;
                            QCursor tmp = _sizeChange;
                            const QPixmap cursorPix = tmp.pixmap().transformed(QTransform().rotateRadians(angle ),Qt::SmoothTransformation);
                            tmp = QCursor(cursorPix);
                            _view->setCursor(tmp);
                            _currentMouseState = MouseStateMoveEdge;
                            _currentEdge = i;
                            _currentTarget = _currentFiles[_currentIndex]->targets[k];
                            found = true;
                            break;
                        }
                    }
                }
            }

            // check if mouse above selected item
            if (!found
                    && _currentFiles[_currentIndex]->targets[k]->boundary->isUnderMouse()
                    && ! (mouseEvent->modifiers() & Qt::ShiftModifier)) {

                if (_currentMouseState != MouseStateMoveItem) {
                    _view->setCursor(Qt::OpenHandCursor);
                    _currentMouseState = MouseStateMoveItem;
                }
                found = true;
                break;
            } else if ((mouseEvent->modifiers() & Qt::ShiftModifier)
                       && _currentFiles[_currentIndex]->targets[k]->boundary->isUnderMouse()) {
                if (_currentMouseState != MouseStateRotateItem) {
                    _view->setCursor(_rotate);
                    _currentMouseState = MouseStateRotateItem;
                    _currentTarget = _currentFiles[_currentIndex]->targets[k];
                }
                found = true;
                break;
            }
        }

        // if not, search for other item to be able to select
        if (!found) {
            for (int k=0; k<(int)_currentFiles[_currentIndex]->targets.size(); k++) {
                if (!_currentFiles[_currentIndex]->targets[k]->boundary->isSelected()
                        && _currentFiles[_currentIndex]->targets[k]->boundary->isUnderMouse()
                        && ! (mouseEvent->modifiers() & Qt::ShiftModifier))
                {
                    if (_currentMouseState != MouseStateSelectItem) {
                        _view->setCursor(Qt::PointingHandCursor);
                        _currentMouseState = MouseStateSelectItem;
                    }
                    found = true;
                }
            }
        }

        if (found == false
                && _currentMouseState != MouseStateNone) {
            if (_findSelectedTarget().size() > 0 && _findSelectedTarget()[0] != 0) {
                _view->setCursor(Qt::ArrowCursor);
                _currentMouseState = MouseStateReleaseItem;
            } else {
                _view->setCursor(Qt::CrossCursor);
                _currentMouseState = MouseStateNone;
            }
        }

    } else if (_currentState == MouseStateRotateItem) {
        QPointF currAngleDir = mouseEvent->screenPos() - _rotationCenter;
        double currAngle = qAtan2(currAngleDir.x(), currAngleDir.y());
        QTransform trans = _lastTransformation;
        _currentTarget->boundary->setTransform(trans.rotateRadians(_lastAngle-currAngle));

    } else if (_currentState == MouseStateMoveEdge) {
        QPointF currCorners[4];
        for (int i=0;i<4; i++) {
            currCorners[i] = _currentTarget->boundary->corners()[i];
        }


        // make sure all computations are done on start basis
        // this will not cause redraw as they are scheduled
        // the next setTransform will cause the actual redraw        
        _currentTarget->boundary->setCorners(_lastCornerPos);
        _currentTarget->boundary->setTransform(_lastTransformation);

        const QPointF posCorner = _currentTarget->boundary->mapFromScene(
                    _view->mapToScene(
                        _view->mapFromGlobal(mouseEvent->screenPos())
                        )
                    );

        const QPointF lastToCurr = _currentTarget->boundary->corners()[_currentEdge] -
                _currentTarget->boundary->corners()[(_currentEdge+3)%4];


        double diff = qAbs(-_lineDist(_lastCornerPos[_currentEdge],
                                     _lastCornerPos[(_currentEdge+1)%4],
                                     posCorner));

        const double lengthSide = _norm(lastToCurr);
        const double distOppositeMouse = qAbs(_lineDist(_lastCornerPos[(_currentEdge+3)%4],
                                                       _lastCornerPos[(_currentEdge+2)%4],
                                                       posCorner));

        if (lengthSide > distOppositeMouse) {
            diff *= -1;
        }

        QPointF movement = _currentTarget->boundary->corners()[(_currentEdge+1)%4] -
                _currentTarget->boundary->corners()[_currentEdge];
        movement /= _norm(movement);
        const double tmp = movement.x();
        movement.setX(movement.y());
        movement.setY(tmp);

        if (_dot(movement, lastToCurr) < 0) {
                movement *= -1.0;
        }

        if (! (mouseEvent->modifiers() & Qt::ControlModifier)) {
            diff /= 2.0;
        }

        QPointF newPos[4];
        newPos[_currentEdge] = _lastCornerPos[_currentEdge] + diff * movement;
        newPos[(_currentEdge+1)%4] = _lastCornerPos[(_currentEdge+1)%4] + diff * movement;
        newPos[(_currentEdge+3)%4] = _lastCornerPos[(_currentEdge+3)%4] - diff * movement;
        newPos[(_currentEdge+2)%4] = _lastCornerPos[(_currentEdge+2)%4] - diff * movement;

        if (mouseEvent->modifiers() & Qt::ControlModifier) {
            // this means scaling around the center
            QTransform trans = _lastTransformation;
            _currentTarget->boundary->setTransform(trans);
            _currentTarget->boundary->update();
        } else {
            // really drag one corner towards somewhere
            // meaning that we translate the object
            QTransform trans = _lastTransformation;
            _currentTarget->boundary->setTransform(trans.translate(diff*movement.x(),
                                                           diff*movement.y()));
            _currentTarget->boundary->update();
        }
        _currentTarget->boundary->setCorners(newPos);


        if (_aspectRatio > 0 ) {
            QPointF c[4]; for (int i=0; i<4; i++) c[i] = _currentTarget->boundary->corners()[i];
            QPointF c0[4]; for (int i=0; i<4; i++) c0[i] = c[i];
            QPointF dir = c[(_currentEdge+1)%4] - c[_currentEdge];
            const double lCurr = _norm(dir);
            dir /= lCurr;
            const double lOrth = _norm(c[(_currentEdge+3)%4] - c[_currentEdge]);

            const double newLength = 0.5 * lOrth * _currAspectRatio;
            QPointF center = c[(_currentEdge+1)%4] + c[_currentEdge];
            center /= 2.0;

            c[(_currentEdge+1)%4] = center + dir * newLength;
            c[_currentEdge] = center - dir * newLength;
            center = c[(_currentEdge+2)%4] + c[(_currentEdge+3)%4];
            center /= 2.0;
            c[(_currentEdge+2)%4] = center + dir * newLength;
            c[(_currentEdge+3)%4] = center - dir * newLength;
            _currentTarget->boundary->setCorners(c);
            _currentTarget->aspect = _aspectRatio;
        }
    } else if (_currentState == MouseStateMoveCorner) {
        // make sure all computations are done on start basis
        // this will not cause redraw as they are scheduled
        // the next setTransform will cause the actual redraw
        _currentTarget->boundary->setCorners(_lastCornerPos);
        _currentTarget->boundary->setTransform(_lastTransformation);

        QPointF posCorner = _currentTarget->boundary->mapFromScene(
                    _view->mapToScene(
                        _view->mapFromGlobal(mouseEvent->screenPos())
                        )
                    );

        QPointF diff = posCorner - _lastCornerPos[_currentCorner];

        if (_aspectRatio > 0) {
            //diff = _lastTransformation.inverted().map(diff);
            QPointF p = posCorner - _lastCornerPos[(_currentCorner+2)%4];
            double currAspect = p.x()/p.y();
            if (currAspect < 0)
                currAspect *= -1.0;
            QPointF d[8];

            if (currAspect > 1.0)
            {
                // landscape: what fits better?
                d[0] = QPointF (p.x(), -p.x() / _aspectRatio);
                d[1] = QPointF (-p.x(), -p.x() / _aspectRatio);
                d[2] = QPointF (-p.x(), p.x() / _aspectRatio);
                d[3] = QPointF (p.x(), p.x() / _aspectRatio);
                d[4] = QPointF (p.y() * _aspectRatio , -p.y());
                d[5] = QPointF (-p.y() * _aspectRatio , -p.y());
                d[6] = QPointF (-p.y() * _aspectRatio , p.y());
                d[7] = QPointF (p.y() * _aspectRatio , p.y());
            }
            else
            {
                // portrait
                d[0] = QPointF (p.x(), -p.x() * _aspectRatio);
                d[1] = QPointF (-p.x(), -p.x() * _aspectRatio);
                d[2] = QPointF (-p.x(), p.x() * _aspectRatio);
                d[3] = QPointF (p.x(), p.x() * _aspectRatio);
                d[4] = QPointF (-p.y() / _aspectRatio, p.y() );
                d[5] = QPointF (-p.y() / _aspectRatio, -p.y() );
                d[6] = QPointF (p.y() / _aspectRatio, -p.y() );
                d[7] = QPointF (p.y() / _aspectRatio, p.y() );
            }

            int minInd=-1;
            double maxDist = 1e10;
            for (int i=0; i<8; i++)
            {
                if (maxDist > _norm2(d[i]-p))
                {
                    minInd = i;
                    maxDist = _norm2(d[i]-p);
                }
            }

            diff = d[minInd] + _lastCornerPos[(_currentCorner+2)%4] - _lastCornerPos[_currentCorner];            
            _currentTarget->aspect = _aspectRatio;
        }


        if (! (mouseEvent->modifiers() & Qt::ControlModifier)) {
            diff /= 2;
        }


        QPointF newPos[4];
        newPos[_currentCorner] = _lastCornerPos[_currentCorner] + diff;
        newPos[(_currentCorner+2)%4] = _lastCornerPos[(_currentCorner+2)%4] - diff;
        newPos[(_currentCorner+3)%4].setX( newPos[(_currentCorner+2)%4].x());
        newPos[(_currentCorner+3)%4].setY( newPos[(_currentCorner)].y());
        newPos[(_currentCorner+1)%4].setX( newPos[(_currentCorner)].x());
        newPos[(_currentCorner+1)%4].setY( newPos[(_currentCorner+2)%4].y());

        if (mouseEvent->modifiers() & Qt::ControlModifier) {
            // this means scaling around the center
            QTransform trans = _lastTransformation;
            _currentTarget->boundary->setTransform(trans);
            _currentTarget->boundary->update();
            _currentTarget->boundary->setCorners(newPos);
            _correctAspectRatio(_currentTarget);
        } else {
            // really drag one corner towards somewhere
            // meaning that we translate the object
            QTransform trans = _lastTransformation;
            _currentTarget->boundary->setTransform(trans.translate(diff.x(), diff.y()));
            _currentTarget->boundary->update();
            _currentTarget->boundary->setCorners(newPos);
        }
    } else if (_currentState == MouseStateNewItem) {
        const QPointF posCorner = _view->mapToScene(
                        _view->mapFromGlobal(mouseEvent->screenPos())
                        );
        _currLine->setLine(QLineF(_lastCornerPos[0], posCorner));
    } else if (_currentState == MouseStateMoveItem) {
        QGraphicsScene::mouseMoveEvent(mouseEvent);
    }
}


void ImageScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
 {
    bool update = false;
    bool recompute = true;
    bool bringCurrentTargetToFront = false;

    if (_currentState == MouseStateMoveItem) {
        _currentState = MouseStateNone;
        _view->setCursor(Qt::OpenHandCursor);
        update = true;

    }
    else if (_currentState == MouseStateNewItem) {
        // fix line and start drawing rectangle for item
        // compute line position & length
        const QPointF firstPoint = _lastCornerPos[0];
        const QPointF currPoint = _view->mapToScene(
                        _view->mapFromGlobal(mouseEvent->screenPos())
                        );

        const double diff = _norm(_view->mapToGlobal(_view->mapFromScene(currPoint)) -
                          _view->mapToGlobal(_view->mapFromScene(firstPoint)));
        if (diff > 5)
        {
            const QPointF diff = currPoint - firstPoint;
            const double length = _norm(diff);

            // create new rect
            emit rotation90(TargetRotation0);
            ImageBoundaryPtr newBoundary (new ImageBoundary());
            _currentTarget = addBoundary(newBoundary);
            _currentTarget->aspect = _aspectRatio;
            newBoundary->setCrop(_cropPercentage);
            newBoundary->setUserHasSeenThis();
            clearSelection();
            newBoundary->setSelected(true);

            // determine rotation and center, for transformation
            // of new boundary
            QPointF newPoints[4];
            const double l2 = length/2.0;
            newPoints[0] = QPointF(0, l2);
            newPoints[1] = QPointF(0, l2);
            newPoints[2] = QPointF(0, -l2);
            newPoints[3] = QPointF(0, -l2);
            qreal angle = qAtan2(diff.y(), diff.x()) - M_PI/2.0;
            newBoundary->setCorners(newPoints);
            const QPointF center = (currPoint + firstPoint)/2.0;
            newBoundary->setTransform(QTransform().translate(center.x(), center.y()).rotateRadians(angle));

            // now prepare new state such that it moves corner
            QCursor tmpCurs = _sizeChange;
            const QPixmap cursorPix = tmpCurs.pixmap().transformed(QTransform().rotateRadians(angle ),Qt::SmoothTransformation);
            tmpCurs = QCursor(cursorPix);
            _view->setCursor(tmpCurs);
            _currentCorner = 0;
            _currentState = MouseStateMoveCorner;
            _currentMouseState = MouseStateNewItem;
            for (int i=0; i<4; i++) {
                _lastCornerPos[i] = _currentTarget->boundary->corners()[i];
            }
            _lastTransformation = _currentTarget->boundary->transform();

            // remove line
            removeItem(_currLine);
            delete _currLine;
            _currLine = 0;
        }

    } else if (_currentState == MouseStateRotateItem) {
        _currentState = MouseStateNone;
        if (!(mouseEvent->modifiers() & Qt::ShiftModifier)) {
            _view->setCursor(Qt::OpenHandCursor);
            _currentMouseState = MouseStateMoveItem;
        }
        update = true;
        bringCurrentTargetToFront = true;
    } else if ( (_currentState == MouseStateMoveCorner
                 || _currentState == MouseStateMoveEdge)
                && _currentMouseState != MouseStateNewItem )
    {
        // if _currentMouseState == MouseStateNewItem
        // this means we have to continue moving the rect
        // until user clicks again
        _currentState = MouseStateNone;
        _currentMouseState = MouseStateMoveItem;
        update = true;
        recompute = true;
        bringCurrentTargetToFront = true;
    } else if  (_currentState == MouseStateMoveEdge
                && _currentMouseState == MouseStateNewItemRelease )
    {
        _currentState = MouseStateNone;
        _currentMouseState = MouseStateMoveItem;
        update = true;
        bringCurrentTargetToFront = true;
    }

    QGraphicsScene::mouseReleaseEvent(mouseEvent);

    if (update) {
        _updateTarget(recompute, true);
    }

    if (bringCurrentTargetToFront) {
        // more than one selection and possibly manipulation
        // an item which is not displayed -> send update image of this image
        QPixmap out;
        if (!_currentTarget->image.isNull()
                && _currentTarget.get() != 0)
        {
            out = QPixmap::fromImage(_currentTarget->image);
        }
        emit updateTargetDisplay(out);
    }
 }

void ImageScene::newRotation(const Rotation90 rot)
{
    QListIterator<TargetImagePtr> i(_findSelectedTarget());
    while (i.hasNext())
    {
        TargetImagePtr tmp = i.next();
        tmp->rotation = rot;
        tmp->boundary->setCopied(false);

    }
    _currGlobalRotation = rot;
    _updateTarget(true);
}

void ImageScene::newIndividualCrop(const double percentage)
{    
    QListIterator<TargetImagePtr> i(_findSelectedTarget());
    while (i.hasNext())
    {
        i.next()->boundary->setCrop(percentage/100.0);
    }
    _updateTarget(true);
    update();
}


void ImageScene::deleteSelection()
{
    if (_currentFiles.size() == 0) {
        return;
    }
    QList<QGraphicsItem *> list = selectedItems();

    _currentState = MouseStateNone;
    _currentMouseState = MouseStateNone;

    for (int i=0; i<list.size(); i++) {
        removeItem(list.at(i));
    }
    for (int i=list.size()-1; i>=0 ; i--) {
        for (int j=0; j<_currentFiles[_currentIndex]->targets.size(); j++) {
            if (list.at(i) == _currentFiles[_currentIndex]->targets[j]->boundary.get()) {
                _currentFiles[_currentIndex]->targets.erase(
                            _currentFiles[_currentIndex]->targets.begin()+j);
            }
        }
    }
    clearSelection();
    _updateTarget();

    nextTarget(0);
    _view->setCursor(Qt::CrossCursor);
}

void ImageScene::nextTarget(const int increment)
{
    if (_currentFiles.size() == 0) {
        return;
    }
    const int numTargets = _currentFiles[_currentIndex]->targets.size();
    int pos = -1;
    if (numTargets>0) {
        const int num = selectedItems().size();
        if (num == 0) {
            if (increment > 0) {
                pos = 0;
            } else {
                pos = numTargets - 1;
            }
            _currentFiles[_currentIndex]->targets[numTargets-1]->boundary->setSelected(true);
        } else {
            int currPos = 0;
            for (int i=0; i<numTargets; i++) {
                if (_currentFiles[_currentIndex]->targets[i]->boundary.get()
                        == selectedItems()[0]) {
                    currPos = i;
                    clearSelection();
                    break;
                }
            }
            pos = currPos+increment < 0 ? numTargets-1 : (currPos+increment) % numTargets;
            _currentTarget = _currentFiles[_currentIndex]->targets[pos];
            _currentTarget->boundary->setSelected(true);
            _updateTarget();
        }
    }
    emit targetPosition(QString(tr("%1 of %2")).arg(pos+1).arg(_currentFiles[_currentIndex]->targets.size()));
}

void ImageScene::newAspectRatio(const double ratio)
{
    _aspectRatio = ratio;

    QListIterator<TargetImagePtr> i(_findSelectedTarget());
    while (i.hasNext())
    {
        _correctAspectRatio(i.next());
    }
    _updateTarget(true);
}

bool ImageScene::_correctAspectRatio(TargetImagePtr target)
{
    target->aspect = _aspectRatio;

    if (_aspectRatio <=0) {
        return false;
    }


    QPointF c[4];
    for (int i=0; i<4; i++) c[i] = target->boundary->corners()[i];

    const double width = _norm(c[0] - c[3]);
    const double height = _norm(c[1] - c[0]);

    const double currAspect = width > height ? (double)width/double(height) : (double)height/double(width);
    if (qAbs(_aspectRatio - currAspect) < 1e-5) {
        return false;
    }

    // fit with short side
    if (width >= height) {
        const double newHeight = 0.5 * width / _aspectRatio;
        QPointF dir, center;
        center = (c[0]+ c[1])/2.0;
        dir = c[0]-c[1];
        dir /= _norm(dir);
        c[0] = center + newHeight * dir;
        c[1] = center - newHeight * dir;

        center = (c[3] + c[2])/2.0;
        c[3] = center + newHeight * dir;
        c[2] = center - newHeight * dir;
        target->boundary->setCorners(c);
    } else {
        const double newWidth = 0.5 * height/ _aspectRatio;
        QPointF dir, center;
        center = (c[0]+ c[3])/2.0;
        dir = c[0]-c[3];
        dir /= _norm(dir);
        c[0] = center + newWidth * dir;
        c[3] = center - newWidth * dir;

        center = (c[1] + c[2])/2.0;
        c[1] = center + newWidth * dir;
        c[2] = center - newWidth * dir;
        target->boundary->setCorners(c);

    }

    return true;
}

void ImageScene::zoomFit()
{
    _view->fitInView(_currentFiles[_currentIndex]->image.get(), Qt::KeepAspectRatio);
}

void ImageScene::zoom1()
{
    if (_view == 0) {
        return;
    }
    QPoint center (_view->width()/2, _view->height()/2);
    QPointF centerScene = _currentFiles[_currentIndex]->image->mapFromParent(center.x(), center.y());
    _view->resetMatrix();
    _view->centerOn(centerScene.x(), centerScene.y());
}


void ImageScene::updateMinMax(const int mi, const int ma)
{
    if ( _backgroundLoader != 0)
        _backgroundLoader->updateMinMax(mi, ma);
}

void ImageScene::_onCopyError(SourceFilePtr source, TargetImagePtr)
{
    QMessageBox::critical(_view,
                          tr("copy error"),
                          tr("Could not copy a target from scanned image '%s'' - enough disc space left ? enough RAM available ? If the images are too large, the 32bit version of this software might also be in trouble").arg(source->source.fileName()));
}

void ImageScene::getIsFirstLast(bool& isFirst, bool& isLast)
{
    isFirst = _currentIndex <= 0;
    isLast = _currentFiles.size() -1 == _currentIndex || _currentFiles.size() == 0;
}

void ImageScene::setEnforceAspect(const bool enforce)
{
    _enforceAspectForNew = enforce;
}

void ImageScene::setDestination(QString destination)
{
    _copyTargetThread.setDestination(destination);
}


void ImageScene::reloadFromNewSettings()
{
    // the target extraction may still work in one of the old images
    // so lock
    while (!ExtractTargets::imageMutex.tryLock(50))
    {
        qApp->processEvents();
    }

    for (int i=0; i<_currentFiles.size(); i++) {
        if (_currentFiles[i]->changed
                && i != _currentIndex) {
            qDebug() << "resetting" << _currentFiles[i]->source.absoluteFilePath();
            _currentFiles[i]->image = QGraphicsPixmapItemPtr();
            _currentFiles[i]->imageOrig = QImage();
            for (int j=0; j<_currentFiles[i]->targets.size(); j++) {
                _currentFiles[i]->targets[j]->image = QImage();
            }
            _currentFiles[i]->targets.clear();
        }
    }
    ExtractTargets::imageMutex.unlock();
    loadPosition();
}

void ImageScene::refreshView()
{
    QMessageBox::StandardButton reply = QMessageBox::question(_view, tr("refresh"),
                                                              tr("This will overwrite currently (and possibly saved) extracted images - continue ?"),
                                                              QMessageBox::Yes | QMessageBox::Abort);
    bool save = reply == QMessageBox::Yes;

    if (save)
    {
        while(!ExtractTargets::imageMutex.tryLock(50))
        {
            qApp->processEvents();
        }

        clear();
        if (_currentIndex >=0) {
            _currentFiles[_currentIndex]->changed = true;
            _currentFiles[_currentIndex]->image = QGraphicsPixmapItemPtr();
            _currentFiles[_currentIndex]->imageOrig = QImage();
            for (int j=0; j<_currentFiles[_currentIndex]->targets.size(); j++) {
                _currentFiles[_currentIndex]->targets[j]->image = QImage();
            }
            _currentFiles[_currentIndex]->targets.clear();
        }
        _currentFiles[_currentIndex]->error = false;
        ExtractTargets::imageMutex.unlock();
        loadPosition();
    }
}

void ImageScene::selectAll()
{
    QListIterator<QGraphicsItem* > i(items());
    while (i.hasNext())
    {
        QGraphicsItem* item = i.next();
        item->setSelected(true);
    }
}

void ImageScene::clear()
{
    // overwrite clear such that it does not delete it
    // otherwise memory leaks with smart pointers
    init();

}

void ImageScene::doneLoadingTarget(const TargetImagePtr &target)
{
    if (_currentIndex >=0 && _currentIndex < _currentFiles.size())
    {
        for (int i=0; i<_currentFiles[_currentIndex]->targets.size(); i++)
        {
            /*QGraphicsItem::GraphicsItemFlags flags =
               _currentFiles[_currentIndex] ->targets[i]->boundary->flags() & (~QGraphicsItem::ItemHasNoContents);
            _currentFiles[_currentIndex]->targets[i]->boundary->setFlags(flags);*/
            if (_currentFiles[_currentIndex]->targets[i]->workOnId == target->workOnId
                    && _currentTarget.get() != 0 && _currentTarget->workOnId == target->workOnId) {
                // the threading model does destroy the rendering -> hotfix re-add
                emit updateTargetDisplay(QPixmap::fromImage(target->image));
                emit setTargetWaiting(false);
            }
            if (_currentFiles[_currentIndex]->targets[i]->workOnId == target->workOnId)
            {
                removeItem(target->boundary.get());
                addItem(target->boundary.get());
            }
        }
    }
    update();
    _view->update();
}

void ImageScene::initialCropChanged(const double val)
{
    _cropPercentage = val;
}
