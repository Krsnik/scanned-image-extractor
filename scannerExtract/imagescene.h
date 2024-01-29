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

#ifndef IMAGESCENE_H
#define IMAGESCENE_H

#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QVector>
#include <QPixmap>
#include <QTransform>
#include <QFileInfoList>
#include <QImage>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QProgressBar>

#include "imageboundary.h"
#include "sourcefile.h"
#include "preloadsource.h"
#include "copytargets.h"
#include "extracttargets.h"

#define DISTANCE_GRAB_CORNER 10
#define DISTANCE_GRAB_EDGE 15
#define NEW_BOX_LINE_COLOR QColor(255,0,0,255)

enum MouseState
{
    MouseStateNotLoaded,
    MouseStateNone,
    MouseStateSelectItem,
    MouseStateReleaseItem,
    MouseStateNewItem,
    MouseStateNewItemRelease,
    MouseStateMoveItem,
    MouseStateRotateItem,
    MouseStateMoveCorner,
    MouseStateMoveEdge,
    MouseStateMoveEdgePre,
};

class ImageScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit ImageScene(QObject *parent = 0);

    ~ImageScene();

    TargetImagePtr addBoundary(ImageBoundaryPtr);

    void init();


signals:
    void updateTargetDisplay(const QPixmap& pixmap);
    void rotation90(const Rotation90 rotation);
    void resetAspect();
    void fileName(const QString&);
    void landscapeButton(bool landscape);
    void changed(bool hasChanges);
    void selectedAspect(float aspect);
    void newCrop(const double cropPerc);
    void noAspect(); // no unique aspect
    void noCrop();
    void noRotation();
    void reloadSettings();
    void renewList(QFileInfoList,int);
    void extractTarget(int, int);
    void setTargetWaiting(bool);
    void filePosition(QString);
    void targetPosition(QString);
    void numSelected(int);
    void doneCopying(const QString&, const QString&);

public slots:

    void newImageList(const QFileInfoList& images = QFileInfoList(),
                      const int selectedIndex = 0);

    void loadPosition(const int newPosition = -1,
                      const bool increment= false,
                      const bool forceReload = false);

    void newRotation(const Rotation90);

    void newIndividualCrop(const double percentage);

    void deleteSelection();

    void nextTarget(const int increment);

    void newAspectRatio(const double ratio);


    void zoom(double factor);

    void zoomFit();
    void zoom1();

    void saveCurrent(const bool noUpdate = false,
                     const bool force = false);

    void updateMinMax(const int, const int);

    void getIsFirstLast(bool &isFirst, bool &isLast);

    void setEnforceAspect(const bool enforce);

    void setDestination(QString destination);

    void setPrefix(const QString prefix) { _copyTargetThread.setPrefix(prefix); }

    void setThresh(const double thresh) { if (hasLoader()) _backgroundLoader->setThresh(thresh); }
    void setMaxAspect(const double maxAspect) {  if (hasLoader())_backgroundLoader->setMaxAspect(maxAspect); }
    void setLevels(const int levels) { if (hasLoader()) _backgroundLoader->setLevels(levels); }
    void setMaxOverlap(const double maxOverlap) { if (hasLoader()) _backgroundLoader->setMaxOverlap(maxOverlap); }
    void setMinArea(const double minArea) { if (hasLoader()) _backgroundLoader->setMinArea(minArea); }
    void setMinAreaWithinImage(const double minArea) { if (hasLoader()) _backgroundLoader->setMinAreaWithinImage(minArea); }
    void setSplitMaxOffset(const double maxOffset) { if (hasLoader()) _backgroundLoader->setSplitMaxOffset(maxOffset); }
    void setSplitMinCornerDist(const double minCornerDist) { if (hasLoader()) _backgroundLoader->setSplitMinCornerDist(minCornerDist); }
    void setSplitMinLengthFrac(const double minLengthFrac) { if (hasLoader()) _backgroundLoader->setSplitMinLengthFrac(minLengthFrac); }
    void setMaximumHierarchyLevel(const int level) { if (hasLoader()) _backgroundLoader->setMaxHierarchyLevel(level); }
    void setNumPreLoad(const int num) { _numNeighbours = num; }
    void reloadFromNewSettings();

    void doneLoadingTarget(const TargetImagePtr& target);

    void refreshView();

    void selectAll();

    void initialCropChanged(const double val);


protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void keyPressEvent ( QKeyEvent * keyEvent );
    void wheelEvent(QGraphicsSceneWheelEvent *event);

private slots:

    void doneCopy(const QString&, const QString&);

    void onSelectionChanged();

    void _doneLoading(const SourceFilePtr &, const int position);

    void _onCopyError(SourceFilePtr, TargetImagePtr);

    void updateSelection();

    void clear();

    void _extractTarget(const int fromImageId,
                        const TargetImagePtr target,
                        const bool highPriority = false);

private:
    QList<TargetImagePtr> _findSelectedTarget();

    inline double _norm(const QPointF&);
    inline double _dot(const QPointF&, const QPointF&);
    inline double _norm2(const QPointF& );
    inline double _lineDist(const QPointF& l1,
                           const QPointF& l2,
                           const QPointF& p);


    void _updateTarget(const bool changed = false, bool dirty = false);

    bool _correctAspectRatio(TargetImagePtr target);

    MouseState _currentState;
    MouseState _currentMouseState;
    QGraphicsView* _view;

    int _currentCorner;
    int _currentEdge;
    TargetImagePtr _currentTarget;

    QPixmap _sizeChange;
    QPixmap _rotate;

    double _lastAngle;
    QPointF _rotationCenter;
    QTransform _lastTransformation;
    QPointF _lastCornerPos[4];

    QVector<SourceFilePtr> _currentFiles;
    int _currentIndex;

    QGraphicsLineItem* _currLine;

    double _cropPercentage;

    Rotation90 _currGlobalRotation;

    double _aspectRatio;
    double _currAspectRatio;

    PreloadSource* _backgroundLoader;
    bool hasLoader() const { return _backgroundLoader != 0; }

    bool _isWaiting;

    CopyTargets _copyTargetThread;

    QTimer _timer;

    bool _enforceAspectForNew;

    QProgressBar* _waitBar;

    QMutex _extractMutex;

    ExtractTargets _targetExtractor;

    int _numNeighbours;
};

#endif // IMAGESCENE_H
