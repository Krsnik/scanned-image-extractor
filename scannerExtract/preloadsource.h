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

#ifndef PRELOADSOURCE_H
#define PRELOADSOURCE_H

#include <QThread>
#include <QString>
#include <QWaitCondition>
#include <QMutex>
#include <QPair>
#include <QVector>

#include <opencv/cv.h>

#include "sourcefile.h"

#include <lbfgs.h>


#define MAX_AREA_OVERLAP 0.3

class PreloadSource : public QThread
{
    Q_OBJECT
public:
    explicit PreloadSource(QObject *parent = 0);

    ~PreloadSource();

    static QMutex preloadMutex;
    static QWaitCondition waitFinished;

    void addLoadFiles(const SourceFilePtr& file,
                      const int position,
                      const bool highPriority=false,
                      const bool onlyLoadImages = false);

    void clear();

    void newTargets(SourceFilePtr& source, cv::Mat &alreadyLoaded, bool &locked);
    QString isCurrentlyLoading() const;

    void updateMinMax(const int, const int);

    void setThresh(const double thresh) { _threshold = thresh; }
    void setMaxAspect(const double maxAspect) {  _maxAspect = maxAspect; }
    void setLevels(const int levels) { _levels = levels; }
    void setMaxOverlap(const double maxOverlap) { _maxOverlap = maxOverlap; }
    void setMinArea(const double minArea) { _minArea = minArea; }
    void setMinAreaWithinImage(const double minArea) { _minAreaWithinImage = minArea; }
    void setSplitMaxOffset(const double maxOffset) { _splitMaxOffsetFrac = maxOffset; }
    void setSplitMinCornerDist(const double minCornerDist) { _splitMinCornerDist = minCornerDist; }
    void setSplitMinLengthFrac(const double minLengthFrac) { _splitMinLengthFrac = minLengthFrac; }
    void setMaxHierarchyLevel(const int level) { _maxHierarchyLevel = level; }

    void setEnergyWeights(const float e1, // border energy
                          const float e2, // area energy
                          const float e3, // aspect energy
                          const float e4) // frame content energy
    { _e1 =e1; _e2 = e2; _e3 = e3; _e4 = e4; }
signals:

    void doneLoading(const SourceFilePtr&, const int position);

public slots:

    void stop();

protected:
    void run();

private:
    void getSumOfRectangleSampling(const QVector<cv::Point2f> pts,
                                          const cv::Mat &image,
                                          double &sumValues,
                                          long &numPixels);

    static void getSumOfRectangleSamplingF(const QVector<cv::Point2f> pts,
                                          const cv::Mat &image,
                                          double &sumValues,
                                          long &numPixels);

    std::vector<cv::Vec3i> getThresholds(const cv::Mat &image, int &bestThresInd);

    void bestRectangles(const cv::Mat& imageOrig,
                        const cv::Mat &boundaries,
                        const cv::Vec3i &thresholds,
                        const std::vector<std::vector< cv::Point > >& contours,
                        const std::vector<cv::Vec4i>& hierarchy,
                        const QVector<cv::RotatedRect>& rects,
                        const QVector<QVector<cv::Point2f> >& points, QVector<double> &energies,
                        QVector<bool>& out); // max aspect ratio is x

    void rectangleOverlap(std::vector<std::vector<cv::Point> > &contours,
                          QVector<cv::RotatedRect> &rects,
                          std::vector<cv::Vec4i>& hierarchy,
                          QVector<QVector<cv::Point2f> > &points,
                          QVector<QVector<float> > &sumOfContourLengths,
                          QVector<bool> &valid) const;

    void computeContourLengths(const std::vector<std::vector<cv::Point> > &contours,
                               const QVector<bool>& valid,
                               QVector<QVector<float> >& sumOfContourLengths);

    double distancePointLine(const cv::Point2f& LP1,
                           const cv::Point2f& LP2,
                           const cv::Point2f& p) const    ;

    void removeFilaments(QVector<QVector<float> > &sumOfContourLengths,
                         const QVector<bool> &valid,
                         std::vector<std::vector< cv::Point > >& contours );

    double polyArea(const QVector<QPointF>& p) const;

    double polyAreaCV(const std::vector<cv::Point>& p,
                      const int from = 0,
                      const int to = -1) const;

    std::vector<cv::RotatedRect> extractRectangles(const std::vector<cv::Vec3i>& thresholds,
                                                   const cv::Mat& image);

    std::vector<cv::Vec2f> extractLines(const std::vector<cv::Vec3i> &thresholds,
                                        const cv::Mat& image);

    /**
     * @brief findHistPeak find position'th peak from left or right
     * @param hist
     * @param fromRight
     * @param windowWidth
     * @param minPercentage
     * @param position
     * @return
     */
    int findHistPeak(const cv::Mat& hist,
                     const bool fromRight,
                     const int windowWidth = 10,
                     const float minPercentage = 0.3,
                     const int position =  0) const;

    void preSelectFast(const cv::Mat &image,
            const std::vector<std::vector< cv::Point > >& contours,
            const std::vector<cv::Vec4i>& hierarchy,
            const QVector<cv::RotatedRect>& rects,
            const QVector<QVector<cv::Point2f> >& points,
            QVector<bool>& valid) const;

    void optimizeRectangle(const cv::Mat& edgeMask,
                           cv::RotatedRect& rectangle);

    bool testAbort(SourceFilePtr sourceFile);

    static lbfgsfloatval_t evaluate(
            void *instance,
            const lbfgsfloatval_t *x,
            lbfgsfloatval_t *g,
            const int n,
            const lbfgsfloatval_t step
            );
    cv::Mat loadAndShrink(const QString& filename);

    QVector<QPair<SourceFilePtr, int> > _filesToLoad;
    SourceFilePtr _current;

    QMutex _fileListMutex;
    QWaitCondition _condition;

    bool _stopped;
    bool _abortAndLoadNext;

    int _min;
    int _max;

    double _threshold;
    double _maxAspect;
    int _levels;
    float _maxOverlap;
    float _minArea;
    float _maxArea;
    float _minAreaWithinImage;
    float _splitMaxOffsetFrac;
    float _splitMinCornerDist;
    float _splitMinLengthFrac;
    int _maxHierarchyLevel;

    QString _currentFilename;

    int _imSize;
    int _histPositions;
    int _histWindowWidth;
    float _histMinPercentage;
    int _histSigma;
    int _cannyTr1;
    int _cannyTr2;
    int _medianBlurMask;
    int _houghRho;
    int _houghAngle;
    float _houghFactor;
    float _rectAngularThresh;
    float _rectMinLineDistFrac;
    float _rectMinDist;
    float _minFrameEnergy;
    double _maxAreaFractionEnergy;
    float _e1;
    float _e2;
    float _e3;
    float _e4;
    int _maxRectOptimizeIterations;
    int _minColorHeight;

    static cv::Mat _currentImageForRectOpt;

    const QImage::Format _qImageFmt;
};

#endif // PRELOADSOURCE_H
