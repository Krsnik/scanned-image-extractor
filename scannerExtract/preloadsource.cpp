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

#include "preloadsource.h"

#include <lbfgs.h>

#include <QMutexLocker>
#include <QDebug>
#include <QtMath>
#include <QDateTime>

#include <opencv/cv.h>
#ifndef OPENCV2
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#else
#include <opencv/highgui.h>
#endif
#include <vector>

//#define DEBUG
//#define DEBUG_TEST
#include "copytargets.h"

#define CvPt2QtPt(p) QPointF(p.x, p.y)

QMutex PreloadSource::preloadMutex;
QWaitCondition PreloadSource::waitFinished;

#ifdef DEBUG_TEST
static int tmpVal2 = 1;
#endif
#ifdef DEBUG
static int tmpVal =0;
#endif

#define ABORT_COMPUTATION(s) \
    if (_abortAndLoadNext || _stopped) return s;

PreloadSource::PreloadSource(QObject *parent)
    : QThread(parent)
    , _stopped(false)
    , _abortAndLoadNext(false)
    , _qImageFmt(QImage::Format_ARGB32)
{
    //setEnergyWeights(5,1,1,5);
}


PreloadSource::~PreloadSource()
{
    _fileListMutex.lock();
    _filesToLoad.clear();
    _stopped = true;
    _condition.wakeOne();
    _fileListMutex.unlock();

    wait();
}

void PreloadSource::addLoadFiles(const SourceFilePtr& file,
                                 const int position,
                                 const bool highPriority,
                                 const bool onlyLoadImages)
{
    QMutexLocker l(&_fileListMutex);
    if (_current && _current->id == file->id) {
        return;
    }
    if (highPriority) {
        _abortAndLoadNext = true;
    }
    _filesToLoad.push_front(QPair<SourceFilePtr, int> (file, onlyLoadImages ? -position-1 : position)); // negative position: onlyLoadImages

    // now remove duplicates (from back)
    for (int i=_filesToLoad.size()-1; i>= 0 ; i--) {
        for (int j=i-1; j >=0; j--) {
            if (_filesToLoad[i].first->source.fileName() == _filesToLoad[j].first->source.fileName()) {
                _filesToLoad.erase(_filesToLoad.begin() + i);
                break;
            }
        }
    }

    _stopped = false;
    if (!isRunning()) {
        if (highPriority) {
            start(QThread::HighPriority);
        }
        else
        {
            start(QThread::LowPriority);
        }
    } else {
        if (highPriority) {
            setPriority(QThread::HighPriority);
        }
        else
        {
            setPriority(QThread::LowPriority);
        }
        _condition.wakeOne();
    }
}

void PreloadSource::clear()
{
    {
        QMutexLocker l(&_fileListMutex);
        _filesToLoad.clear();
    }
    if (!isRunning()) {
        start(QThread::LowPriority);
    } else {
        _condition.wakeOne();
    }
    //waitFinished.wait(&preloadMutex);
}

inline double PreloadSource::polyArea(const QVector<QPointF>& p) const
{
    //https://en.wikipedia.org/wiki/Polygon#Area_and_centroid
    const int n = p.size();
    double area = 0.0;
    for (int i=0; i<n; i++)
    {
        area += p[i].x()*p[(i+1)%n].y() - p[(i+1)%n].x()*p[i].y();
    }
    if (area < 0)
        return -0.5*area;
    else
        return 0.5*area;
}


inline double PreloadSource::polyAreaCV(const std::vector<cv::Point> &p,
                                        const int from,
                                        const int to) const
{
    //https://en.wikipedia.org/wiki/Polygon#Area_and_centroid
    const int n = p.size();
    const int localTo = to < 0 ? n : qMin(n, to);
    double area = 0.0;
    if (from < to) {
        for (int i=from; i<localTo; i++)
        {
            const int toInd = i == localTo -1 ? from : i+1;
            area += p[i].x*p[toInd].y - p[toInd].x*p[i].y;
        }
    } else {
        // with walk over 0
        for (int i=0; i<=from; i++)
        {
            const int toInd = i == from ? to : i+1;
            area += p[i].x*p[toInd].y - p[toInd].x*p[i].y;
        }
        for (int i=localTo; i<n; i++)
        {
            const int toInd = i == n-1 ? 0 : i+1;
            area += p[i].x*p[toInd].y - p[toInd].x*p[i].y;
        }
    }
    if (area < 0)
        return -0.5*area;
    else
        return 0.5*area;
}

void PreloadSource::getSumOfRectangleSampling(const QVector<cv::Point2f> pts,
                                              const cv::Mat& image,
                                              double& sumValues,
                                              long int& numPixels)
{
    // assume single channel image
    sumValues = 0;
    numPixels = 0;

    IplImage tmp = cvIplImage(image);
    assert(image.type() == CV_8U);

    for (int i=0; i<4; i++)
    {

        CvLineIterator iterator;
        const int count = cvInitLineIterator(
            &tmp,
            cvPoint(cvRound(pts[i].x), cvRound(pts[i].y)),
            cvPoint(cvRound(pts[(i + 1) % 4].x), cvRound(pts[(i + 1) % 4].y)),
            &iterator,
            4
        );

        for( int j = 0; j < count; j++ ){
            sumValues += iterator.ptr[0];
            CV_NEXT_LINE_POINT(iterator);
        }
        /* for float images:
        cv::Point2f diff = pts[(i+1)%4] - pts[i];
        const int count= cv::norm(diff);
        for (int j=0; j<count; j++) {
            cv::Point2f p2(pts[i].x + diff.x * float(j) / (float)count,
                          pts[i].y + diff.y * float(j) / (float)count);
            cv::Point2i p((int)p2.x, (int)p2.y);
            if (p.x >=0 && p.y >=0
                    && p.x < image.size().width
                    && p.y < image.size().height)
            {
                sumValues += image.at<float>(p.y, p.x);
            }
        } */
        numPixels += cv::norm(pts[i] - pts[(i+1)%4]);
    }
}

void PreloadSource::getSumOfRectangleSamplingF(const QVector<cv::Point2f> pts,
                                              const cv::Mat& image,
                                              double& sumValues,
                                              long int& numPixels)
{
    // assume single channel image
    sumValues = 0;
    numPixels = 0;

    assert(image.type() == CV_32F);

    for (int i=0; i<4; i++)
    {

        // for float images:
        cv::Point2f diff = pts[(i+1)%4] - pts[i];
        const int count= cv::norm(diff);
        for (int j=0; j<count; j++) {
            cv::Point2f p2(pts[i].x + diff.x * float(j) / (float)count,
                          pts[i].y + diff.y * float(j) / (float)count);
            cv::Point2i p((int)p2.x, (int)p2.y);
            if (p.x >=0 && p.y >=0
                    && p.x < image.size().width
                    && p.y < image.size().height)
            {
                const float val = image.at<float>(p.y, p.x);
                sumValues += val*0 == 0? val : 0;
            }
        }
        numPixels += cv::norm(pts[i] - pts[(i+1)%4]);
    }
}

struct QPairFirstComparer
{
    template<typename T1, typename T2>
    bool operator()(const QPair<T1,T2> & a, const QPair<T1,T2> & b) const
    {
        return a.first > b.first;
    }
};

void PreloadSource::preSelectFast(const cv::Mat& image,
        const std::vector<std::vector< cv::Point > >& contours,
        const std::vector<cv::Vec4i>& hierarchy,
        const QVector<cv::RotatedRect>& rects,
        const QVector<QVector<cv::Point2f> >& points,
        QVector<bool>& valid) const
{
    const float imageArea = image.rows * image.cols;
    int num =0;
#pragma omp parallel for
    for (unsigned int i=0; i< contours.size(); i++)
    {
        if (!valid[i])
            continue;

        int level = 0;
        int parent = hierarchy[i][3];
        while (parent >= 0)
        {
            level++;
            parent = hierarchy[parent][3];
            if (level > _maxHierarchyLevel) {
                break;
            }
        }
        if (rects[i].size.area() / (double) imageArea < _minArea
                /*|| ratio > _maxAspect do not check aspect since we may want to split this rectangle
                  which might result in a better aspect */
                //a < _minAreaWithinImage
                || level > _maxHierarchyLevel)
        {
#pragma omp critical
            valid[i] = false;
            num++;
        }
    }
}

#ifdef DEBUG
static int extractNum = 0;
#endif

inline float extractRectangleValueSum(const cv::Mat& image,
                                       const cv::RotatedRect rect)
{
    if (rect.size.area() <20) {
        return 0;
    }
    cv::Mat M, rotated, cropped;
    float angle = rect.angle;
    cv::Size rect_size = rect.size;

    // thanks to http://felix.abecassis.me/2011/10/opencv-rotation-deskewing/
    if (rect.angle < -45.) {
        angle += 90.0;
        qSwap(rect_size.width, rect_size.height);
    }
    M = getRotationMatrix2D(rect.center, angle, 1.0);

    // perform the affine transformation
    cv::warpAffine(image, rotated, M, image.size(), cv::INTER_LINEAR);

    // crop the resulting image
    cv::getRectSubPix(rotated, rect_size, rect.center, cropped);
    const float out = cv::sum(cropped)[0];
#ifdef DEBUG
    cv::imwrite(QString("test_extract%1_e%2.png").arg(extractNum++).arg(out/(float)rect.size.area()).toLocal8Bit().toStdString(), cropped);
#endif
    return out;
}

void PreloadSource::bestRectangles(const cv::Mat& imageOrig,
                                   const cv::Mat& boundaries,
                                   const cv::Vec3i &thresholds,
                                   const std::vector<std::vector< cv::Point > >& contours,
                                   const std::vector<cv::Vec4i>& hierarchy,
                                   const QVector<cv::RotatedRect>& rects,
                                   const QVector<QVector<cv::Point2f> >& points,
                                   QVector<double>& energies,
                                   QVector<bool> &out)
{
    _minFrameEnergy = 0.75;
    _maxAreaFractionEnergy = 0.5;
    _e1 = 0.1;
    _e2 = 5.0;
    _e3 = 1;
    _e4 = 10;

#ifdef DEBUG
    cv::Mat bdCopy;
    boundaries.copyTo(bdCopy);
    cv::cvtColor(bdCopy, bdCopy, CV_GRAY2BGR);
#endif
    const int imSize = 500;
    cv::Mat smallImage;
    // downsample very small, so it is feasible to extract whole rectangles

    // there's usually a light peak from the scanner background
    // hence compute energy better = higher difference from background
    // this will be done later, however, he we prepare the corresponding image

    float scale;
    if (imageOrig.size().width < imageOrig.size().height)
    {
        scale = (float) imageOrig.size().height / (float) imSize;
        cv::resize(imageOrig, smallImage, cv::Size(int(float(imageOrig.size().width)/scale),
                                                   imSize));
    }
    else
    {
        scale = (float) imageOrig.size().width / (float) imSize;
        cv::resize(imageOrig, smallImage, cv::Size(imSize,
                                       int(float(imageOrig.size().height)/scale)));
    }

    cv::Mat diffFromScannerBackground,
            difference = cv::Mat::zeros(smallImage.size(), CV_8U) * 255.0,
            differenceDark = cv::Mat::zeros(smallImage.size(), CV_8U) * 255.0,
            tmp, backgroundEgdes;

    std::vector<cv::Mat> channels(3);
    cv::split(smallImage, channels);
    for (int i=0; i<3; i++) {
        diffFromScannerBackground = cv::Mat::ones(smallImage.size(), CV_8U) * thresholds[i];
        // bright scanner background
        cv::threshold(channels[i], tmp, thresholds[i], 0, cv::THRESH_TRUNC);
        cv::absdiff(diffFromScannerBackground, tmp, diffFromScannerBackground);
        difference = cv::max(difference, diffFromScannerBackground);


        // black scanner background
        diffFromScannerBackground = cv::Mat::ones(smallImage.size(), CV_8U) * 15; //thresholds[_histPositions][i];
        cv::absdiff(diffFromScannerBackground, channels[i], diffFromScannerBackground);
        differenceDark = cv::max(differenceDark, diffFromScannerBackground);
    }

    // * don't use the black values for now - many images seem dark and that would exclude quite some of them
    // difference = cv::min(differenceDark, difference);

    cv::medianBlur(difference, difference, 3);
    const int threshTo = 255;
    difference = difference * (threshTo / 50);

    const int s = 3;
    cv::Mat kernel = cv::getStructuringElement(0, cv::Size(s,s));
    cv::Canny(difference, backgroundEgdes, 150, 300);
    const int w = backgroundEgdes.size().width-1,  h = backgroundEgdes.size().height-1;
    cv::line(backgroundEgdes, cv::Point2i(0,0), cv::Point2i(w,0), cv::Scalar(255,2555,255,255));
    cv::line(backgroundEgdes, cv::Point2i(0,0), cv::Point2i(0,h), cv::Scalar(255,2555,255,255));
    cv::line(backgroundEgdes, cv::Point2i(w,0), cv::Point2i(w,h), cv::Scalar(255,2555,255,255));
    cv::line(backgroundEgdes, cv::Point2i(0,h), cv::Point2i(w,h), cv::Scalar(255,2555,255,255));
    cv::dilate(backgroundEgdes, backgroundEgdes, kernel);
    cv::GaussianBlur(backgroundEgdes, backgroundEgdes, cv::Size(0,0), 3.5);

    cv::threshold(difference, difference, 127, 255, cv::THRESH_BINARY);


#ifdef DEBUG_TEST
    cv::imwrite(QString("test_smallCanny%1.png").arg(tmpVal2).toLocal8Bit().toStdString(), backgroundEgdes);
    cv::imwrite(QString("test_smallImDiff%1.png").arg(tmpVal2++).toLocal8Bit().toStdString(), difference);
#endif
#ifdef DEBUG
    cv::imwrite(QString("test_smallIm%1.png").arg(tmpVal).toLocal8Bit().toStdString(), smallImage);
#endif

    int n = contours.size();
    QVector<int> selected(n); // 0 not considered, 1 selected, -1 disregarded
    for(int i=0; i< n; i++)
    {
        selected[i] = out[i] ? 0 : -1;
    }
    energies.resize(n);

#define INVALIDATE(i) \
    out[i] = false; \
    selected[i] = -1;

    QVector<QPair<float, int> > energiesAndIdexes(n); // for sorting with backmap

    const double imageArea = imageOrig.rows * imageOrig.cols;

    // compute energy
#pragma omp parallel for
    for (int i=0; i<n; i++)
    {
        if (!_stopped && !_abortAndLoadNext)
        {
            n = contours.size();
            bool found = false;
            double energy = 0;
            if (out[i]) {
                // ensure maximum aspect ratio ?
                const double ratio = rects[i].size.width > rects[i].size.height ?
                            (double)rects[i].size.width / (double)rects[i].size.height :
                            (double)rects[i].size.height / (double)rects[i].size.width;

                // rectangle is too much outside of the image?
                QPolygonF pIm, pRect;
                const float x = imageOrig.size().width, y = imageOrig.size().height;
                pRect << CvPt2QtPt(points[i][0]) << CvPt2QtPt(points[i][1])
                        << CvPt2QtPt(points[i][2]) << CvPt2QtPt(points[i][3]);
                pIm << CvPt2QtPt(cv::Point2f(0,0)) << CvPt2QtPt(cv::Point2f(x,0))
                    << CvPt2QtPt(cv::Point2f(x,y)) << CvPt2QtPt(cv::Point2f(0,y));
                QPolygonF intersected = pRect.intersected(pIm);
                const double withinImageArea = polyArea(intersected);
                const double a = withinImageArea / rects[i].size.area();

                int level = 0;
                int parent = hierarchy[i][3];
                while (parent >= 0)
                {
                    level++;
                    parent = hierarchy[parent][3];
                }

                // all together + min Area
                if (rects[i].size.area() / (double) imageArea > _minArea
                        && rects[i].size.area() / (double) imageArea < _maxArea
                        && ratio <= _maxAspect
                        && out[i]
                        && a > _minAreaWithinImage
                        && level <= _maxHierarchyLevel)
                {

                    const cv::RotatedRect& r = rects[i];
                    const cv::RotatedRect rectSmall(cv::Point2f(float(r.center.x)/scale,
                                                                float(r.center.y)/scale),
                                                    cv::Size2f(float(r.size.width)/scale,
                                                               float(r.size.height)/scale),
                                                    r.angle);
                    const double frameEnergy = extractRectangleValueSum(difference, rectSmall)
                            / double((double)threshTo * (double)rectSmall.size.area());

                    if (frameEnergy > _minFrameEnergy)
                    {
                        double sumValues, backgroundSum;
                        long int numPixels, backgroundNum;
                        getSumOfRectangleSampling(points[i], boundaries, sumValues, numPixels);
                        getSumOfRectangleSampling(points[i], backgroundEgdes, backgroundSum, backgroundNum);
#ifdef DEBUG
                        QVector<cv::Point2f> pts(4);
                        rectSmall.points(pts.data());
                        for (int k=0; k<4; k++) {
                            cv::line(bdCopy, points[i][k], points[i][(k+1)%4], cv::Scalar(0,0,255));
                            cv::line(smallImage, pts[k], pts[(k+1)%4], cv::Scalar(0,0,255));
                        }
#endif
                        // best area is around half of the image
                        // energy: 1. sampling values along the field
                        //         2. area (highest value is half the image)
                        //         3. aspect - 7:5 (between 3:2 and 4:3)
                        //         4. monotony inside rect (used above the reject background rectangles)

                        const double areaEnergy = (imageArea*_maxAreaFractionEnergy - qAbs(double(imageArea*_maxAreaFractionEnergy - rects[i].size.area()))) / (imageArea*_maxAreaFractionEnergy);
                        const double borderEnergy = sumValues / (double)numPixels / 255.0;
                        const double borderEnergyBackground = backgroundSum / (double)backgroundNum / 255.0;
                        const double aspectEnergy = (7.0/5.0 - qAbs(7.0/5.0 - ratio)) * 5.0 / 7.0;                        
                        energy = _e1 * areaEnergy * areaEnergy
                                + _e2 * qPow(borderEnergy, 2.0)
                                + _e2 * qPow(borderEnergyBackground, 2.0)
                                + _e3 * aspectEnergy * aspectEnergy
                                + _e4 * frameEnergy * frameEnergy;

                        /*
                        double tmp = areaEnergy, tmp2 = aspectEnergy, pow =2.0;
                        energy = _e1 * qPow(0.2*qRound(tmp*5.0), pow)
                                + _e2 * qPow(qMin(borderEnergy, frameEnergy ), pow)
                                + _e4 * qPow(0.2*qRound(tmp2*5.0), pow);*/

                        found = true;
                    }
                }
            }
#pragma omp critical
            if (found) {
                energiesAndIdexes[i] = QPair<float,int>(energy, i);
                energies[i] = energy;
            }
            else
            {
                energiesAndIdexes[i] = QPair<float,int>(0, i);
                INVALIDATE(i);
            }
        }
    }
    // Ordering ascending
    qSort(energiesAndIdexes.begin(), energiesAndIdexes.end(), QPairFirstComparer());

    // now choose: best first, then disable all parents and kids. search next
    int choice = 0;
    while (choice >= 0 && !_stopped && !_abortAndLoadNext)
    {
        choice = -1;
        // search next best rectangle
        for (int i=0; i<n; i++)
        {
            if (selected[energiesAndIdexes[i].second] == 0) {
                choice = energiesAndIdexes[i].second;
                break;
            }
        }

        if (choice <=0) {
            break;
        }
        selected[choice] =  1;
        out[choice] = true;
        // hierarchy[i]: next(0), prev(1), firstKid(2), parent(3)

        // look for parents
        int parent = hierarchy[choice][3];
        while(parent >=0)
        {
            INVALIDATE(parent);
            parent = hierarchy[parent][3];
        }

        // now stack kids and work through
        QList<int> kids;
        if (hierarchy[choice][2]>=0)
        {
            kids.append(hierarchy[choice][2]);
        }
        while (kids.length()>0)
        {
            const int current = kids.first();
            kids.pop_front(); // remove that element
            if (current < 0)// || selected[current] <0)
            {
                continue;
            }

            INVALIDATE(current);

            // append first of that current kid
            kids.append(hierarchy[current][2]);

            // append next neighbour of that current kid
            kids.append(hierarchy[current][0]);
        }
    }

#ifdef DEBUG
    cv::imwrite(QString("test_rectsSampling%1.png").arg(tmpVal).toLocal8Bit().toStdString(), bdCopy);
    cv::imwrite(QString("test_rectsSamplingSmall%1.png").arg(tmpVal).toLocal8Bit().toStdString(), smallImage);
#endif
}

inline QVector<float> computeContourLength(const std::vector<cv::Point>& contour)
{
    const int m = contour.size();
    // compute sum of contour lengths
    QVector<float> length(m+1);
    for (int j=1; j<m; j++)
    {
        length[j] = length[j-1] + cv::norm(contour[j] - contour[j-1]);
    }
    length[m] = length[m-1] + cv::norm(contour[0] - contour[m-1]);
    return length;
}

inline void addSubRectangles(const int i,
                             const int i1,
                             const int i2,
                             std::vector<std::vector< cv::Point > >& contours,
                             QVector<cv::RotatedRect>& rects,
                             std::vector<cv::Vec4i>& hierarchy,
                             QVector<QVector<cv::Point2f> >& points,
                             QVector<bool>& valid)
{

    // ok here it seems there is a possibility such that two rectangles are merged into one
    // now add these two sub-rectangles/contours and make sure hierarchy is just correct
    // (however the "inside" relation won't be correct)

    // hierarchy[i]: next(0), prev(1), firstKid(2), parent(3)

    const int l= contours.size();
    std::vector< cv::Point > c1, c2;
    c1.insert(c1.end(), contours[i].begin(), contours[i].begin() + (i1==(int)contours[i].size()-1? i1: i1+1));
    c1.insert(c1.end(), contours[i].begin() + i2, contours[i].end()); //(i2==0? i2 : i2-1), contours[i].end() );

    c2.insert(c2.end(),
              contours[i].begin() + i1, //(i1==0?i1:i1-1),
              contours[i].begin() + (i2==(int)contours[i].size()-1? i2: i2+1));
    contours.push_back(c1);
    contours.push_back(c2);

    rects.push_back(minAreaRect (c1));
    rects.push_back(minAreaRect (c2));

    valid.push_back(true);
    valid.push_back(true);

    points.resize(l+2);
    points[l].resize(4);
    points[l+1].resize(4);
    rects[l].points(points[l].data());
    rects[l+1].points(points[+1l].data());

    hierarchy.push_back(cv::Vec4i(l+1, -1, -1, i));
    if (hierarchy[i][2] >= 0)
    {
        const int kid = hierarchy[i][2];
        hierarchy[kid][1] = l+1;

        hierarchy.push_back(cv::Vec4i(kid, l, -1, i));
    }
    else
    {
        hierarchy.push_back(cv::Vec4i(-1, l, -1, i));
    }
    hierarchy[i][2] = l;
}

void PreloadSource::rectangleOverlap(std::vector<std::vector< cv::Point > >& contours,
                                     QVector<cv::RotatedRect>& rects,
                                     std::vector<cv::Vec4i>& hierarchy,
                                     QVector<QVector<cv::Point2f> >& points,
                                     QVector<QVector<float> > &sumOfContourLengths,
                                     QVector<bool>& valid) const
{

    // find rectangles which (slightly) overlap at two respective corners
    // also look for single large convexity defects
    const int n = contours.size();
    for (int i=0; i<n && !_stopped && !_abortAndLoadNext; i++)
    {
        if (!valid[i])
            continue;

        std::vector<cv::Vec4i> defects;
        std::vector<int> hull;

        cv::convexHull(contours[i], hull);

        if (hull.size() < 4)
        {
            continue;
        }

        cv::convexityDefects(contours[i], hull, defects);

        if (defects.size() > 1)
        {
            // find best defect pair, which means closest distance to each other
            // and long enough contour length in between
            const double rectLength = 2*rects[i].size.width + 2*rects[i].size.height;
            const int m = contours[i].size();

            // now compare defects, but only if contour in between is long enough
            for (unsigned int j=0; j < defects.size(); j++)
            {
                if (sumOfContourLengths[i].size() < (int)contours[i].size()) {
                    sumOfContourLengths[i] = computeContourLength(contours[i]);
                }
                const QVector<float>& length  = sumOfContourLengths[i];

                // getMaxDefect position
                const double maxDist = qMin(rects[i].size.width,
                                            rects[i].size.height)/2.0;
                const int bestN = 5;
                QVector<double> maxDistFound (bestN, 0);
                QVector<QPair<int, int> > bestPairs(bestN);
                /**
                  * compare to closest point on rectangle. if it is only (say half)
                  * the length away (-> to the opposite side) then add splitted
                  */
                //const double minLengthInBetween = 40.0;
                //const double maxAreaPerLength = 5.0;
                const int ind = defects[j][2];
                for (unsigned int j=0; j<contours[i].size(); j++)
                {
                    const int i1 = qMin(ind, (int)j), i2 = qMax(ind, (int)j);
                    const double d1 = length[i2] - length[i1];
                    const double d2 = length[i1] + (length[m] - length[i1]);
                    double shortestDist = qMin(d1, d2);
                    if (cv::norm(contours[i][i1] - contours[i][i2]) < maxDist
                            && shortestDist > rectLength * _splitMinLengthFrac)
                    {
                        // energy consists of length fraction and distance to rectangle fraction
                        // so defects with rather equal lengths towards both circles
                        // and a high defect values are preferred
                        const double distL1P1 = distancePointLine(points[i][0], points[i][1], contours[i][i1]);
                        const double distL1P2 = distancePointLine(points[i][2], points[i][3], contours[i][i1]);
                        const double distL2P1 = distancePointLine(points[i][1], points[i][2], contours[i][i1]);
                        const double distL2P2 = distancePointLine(points[i][0], points[i][3], contours[i][i1]);
                        double minRectDist = qMin(distL1P1, qMin(distL1P2, qMin(distL2P1, distL2P2)));

                        // try to distribute around the rectangle
                        const double energy = (shortestDist - length.last() / (double)bestN)
                                + 4 * minRectDist * rectLength;
                        int choice = bestN;
                        for (int k=bestN-1; k>=0; k--) {
                            if (energy < maxDistFound[k] )
                            {
                                break;
                            }
                            choice = k;
                        }
                        if (choice < bestN) {
                            bestPairs[choice] = QPair<int, int>(i1, i2);
                            maxDistFound[choice] = energy;
                        }
                    }
                }
                for (int k=0; k<bestN; k++) {
                    if (maxDistFound[k] > 0) {
                        addSubRectangles(i, bestPairs[k].first, bestPairs[k].second, contours, rects, hierarchy, points, valid);
                    }
                }

                /**
                 * the following seems to work ok, however it is somewhat limited
                 * since it is only tried to compare to neighboring defects
                 */
                // compare to other defects
                for (unsigned int k=j+1; k < defects.size(); k++)
                {
                    const int i1 = qMin(defects[j][2], defects[k][2]);
                    const int i2 = qMax(defects[j][2], defects[k][2]);
                    double shortestDist = qMin(length[i2] - length[i1],
                                               length[i1] + (length[m] - length[i1]));
                    if (shortestDist > rectLength * _splitMinLengthFrac
                            && cv::norm(contours[i][i1] - contours[i][i2]) < maxDist)
                    {
                        // now save this
                        // found best defect pair
                        // now test how close that pair is and if it is rather on the diagonal of the bounding rect
                        const double diagLength = qSqrt(rects[i].size.width * rects[i].size.width
                                                        + rects[i].size.height * rects[i].size.height);
                        const double distD1P1 = distancePointLine(points[i][0], points[i][2], contours[i][i1]);
                        const double distD1P2 = distancePointLine(points[i][0], points[i][2], contours[i][i2]);
                        const double distD2P1 = distancePointLine(points[i][1], points[i][3], contours[i][i1]);
                        const double distD2P2 = distancePointLine(points[i][1], points[i][3], contours[i][i2]);

                        const double sumDiagDist = qMin( distD1P1 + distD1P2, distD2P1 + distD2P2);

                        if (sumDiagDist < _splitMaxOffsetFrac * diagLength)
                        {
                            // also make sure this is far enough away from corner
                            double distanceCorner = 1e10;
                            for (int l=0; l<4; l++)
                            {
                                distanceCorner = qMin(distanceCorner, cv::norm(points[i][l]));
                            }
                            if (distanceCorner > _splitMinCornerDist * diagLength) {
                                addSubRectangles(i, i1, i2, contours, rects, hierarchy, points, valid);
                            }
                        }
                    }
                }
            }
        }
    }
}

/*
void PreloadSource::computeContourLengths(const std::vector<std::vector< cv::Point > >& contours,
                                          const QVector<bool>& valid,
                                          QVector<QVector<float> >& sumOfContourLengths)
{
    const int n = contours.size();
    sumOfContourLengths.resize(n);
#pragma omp parallel for
    for (int i=0; i<n; i++)
    {
        if (valid[i])
        {
            const int m = contours[i].size();
            // compute sum of contour lengths
            QVector<float> length(m+1);
            for (int j=1; j<m; j++)
            {
                length[j] = length[j-1] + cv::norm(contours[i][j] - contours[i][j-1]);
            }
            length[m] = length[m-1] + cv::norm(contours[i][0] - contours[i][m-1]);

    #pragma omp critical
            sumOfContourLengths[i] = length;
        }
    }
}
*/

void PreloadSource::removeFilaments(QVector<QVector<float> >& sumOfContourLengths,
                                    const QVector<bool> & valid,
                                    std::vector<std::vector< cv::Point > >& contours)
{
    /*
     *I did remove this since there's quite some danger correct images might get removed, as well
     */
    const int n = contours.size();
#pragma omp parallel for
    for (int i=0; i<n; i++)
    {
        if (!valid[i]) {
            continue;
        }

        if (sumOfContourLengths[i].size() < (int)contours[i].size()) {
            sumOfContourLengths[i] = computeContourLength(contours[i]);
        }

        const int m = contours[i].size();
        QVector<bool> valid(m, true);

        // compute all distances
        for (int j=1; j<m; j++)
        {
            if (valid[j])
            {
                for (int k=j+1; k<m; k++)
                {
                    if (valid[k])
                    {
                        double dist = cv::norm(contours[i][j] - contours[i][k]);
                        const double maxDist = 10.0;
                        const double minLengthInBetween = 40.0;
                        const double maxAreaPerLength = 1.0;

                        if (dist <= maxDist) {
                            const double cLength1 = sumOfContourLengths[i][k] -
                                                                sumOfContourLengths[i][j];
                            if (cLength1 >= minLengthInBetween) {
                                // now compute area of this part polygon.
                                // if it is very small, then add this pair to removal.
                                // sample both directions
                                const double areaPerLength1 =
                                            polyAreaCV(contours[i], j, k ) / cLength1;
                                if (areaPerLength1 < maxAreaPerLength)
                                {
                                    // prevent computing this, again
                                    for (int v=j+1; v<k; v++) valid[v] = false;
                                }
                            }
                            const double cLength2 = sumOfContourLengths[i][j] +
                                    (sumOfContourLengths[i].last() - sumOfContourLengths[i][j]);
                            if (cLength2 >= minLengthInBetween)
                            {
                                const double areaPerLength2 =
                                            polyAreaCV(contours[i], k, j ) / cLength2;
                                if (areaPerLength2 < maxAreaPerLength)
                                {
                                    // prevent computing this, again
                                    for (int v=k+1; v<m; v++) valid[v] = false;
                                    for (int v=0; v<j; v++) valid[v] = false;
                                }
                            }
                        }
                    }
                }
            }
        }
#pragma omp critical
        {
            for (int j=m-1; j>=0; j--) {
                if (!valid[j]) {
                    contours[i].erase(contours[i].begin() + j);
                }
            }
        }
    }
    /**/
}

double PreloadSource::distancePointLine(const cv::Point2f& LP1,
                                      const cv::Point2f& LP2,
                                      const cv::Point2f& p) const
{
    const double dY = LP2.y - LP1.y;
    const double dX = LP2.x - LP1.y;
    const double ret = qAbs ( dY * p.x - dX * p.y + LP2.x * LP1.y - LP2.y * LP1.x) / qSqrt(dY * dY + dX * dX);
    return ret;
}


cv::Mat PreloadSource::loadAndShrink(const QString& filename)
{
    cv::Mat image = cv::imread(filename.toLocal8Bit().data(),
                               CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);

    if (image.channels() == 1) {
        cv::cvtColor(image, image, CV_GRAY2BGR);
    }
    if (image.channels() == 4) {
        cv::cvtColor(image, image, CV_BGRA2BGR);
    }

    if (image.depth() == CV_16S
            || image.depth() == CV_16U)
    {
        qDebug() << "found 16bit image";
        cv::normalize(image,image,0,255,cv::NORM_MINMAX);
        image.convertTo(image, CV_8U);
    }
    return image;
}

void PreloadSource::newTargets(SourceFilePtr &source, cv::Mat& alreadyLoaded, bool& locked)
{
    source->targets.clear();
    _imSize = 2000;

    // prevent two big chunks of memory at the same time -> see copytargets    

    if (alreadyLoaded.empty())
    {
        alreadyLoaded = loadAndShrink(source->source.absoluteFilePath());
    }

    cv::Mat image = alreadyLoaded;

    if (image.size().width == 0
            || image.size().height == 0
            || image.empty())
    {
        source->error = true;
        return;
    }

    float scale;
    cv::Mat im;
    if (image.size().width < image.size().height)
    {
        scale = (float) image.size().height / (float) _imSize;
        cv::resize(image, im, cv::Size(int(float(image.size().width)/scale),
                                       _imSize));
    }
    else
    {
        scale = (float) image.size().width / (float) _imSize;
        cv::resize(image, im, cv::Size(_imSize,
                                       int(float(image.size().height)/scale)));
    }
    CopyTargets::memorySaveMutex.unlock();
    locked = false;
    ABORT_COMPUTATION();

    int bestThresh;
    std::vector<cv::Vec3i> thresholds = getThresholds(image, bestThresh);
    std::vector<cv::RotatedRect> rectDetection = extractRectangles(thresholds, image);
    cv::cvtColor(im, image,  cv::COLOR_BGR2GRAY);

    cv::medianBlur(image, image, 5);

    image.convertTo(image, CV_32F);

    if (image.cols == 0) {
        return;
    }
    cv::Mat overallMask, out;
    const int levels = _levels;

    overallMask = cv::Mat::zeros(image.rows, image.cols, image.type());

    for (int i=0; i < levels; i++)
    {
        ABORT_COMPUTATION()
        cv::pyrDown(image, image);
        cv::Mat mu, mu2, sigma;
        cv::blur(image, mu, cv::Size(3,3));
        cv::Mat tmp(image.rows, image.cols, image.type());

        cv::multiply(image, image, tmp);
        cv::blur(tmp, mu2, cv::Size(3,3));
        cv::multiply(mu, mu, tmp);
        cv::sqrt(mu2 - tmp, sigma);

        cv::resize(sigma, tmp, cv::Size(overallMask.cols, overallMask.rows));
        overallMask += tmp;
    }
    image.release();

    // remove NaN and INF
    for(int row = 0; row < overallMask.rows; ++row) {
        float* p = overallMask.ptr<float>(row);
        for(int col = 0; col < overallMask.cols; col++) {
             if (p[col] * 0.0f != 0.0f) {
                 p[col] = 0.0f;
             }
        }
    }

    ABORT_COMPUTATION()


    const int s = 5;
    cv::Mat kernel = cv::getStructuringElement(0, cv::Size(s,s));
    cv::morphologyEx(overallMask, overallMask, cv::MORPH_OPEN, kernel);
    cv::erode(overallMask, overallMask, kernel);

    cv::threshold(overallMask, out, _threshold*levels, 1, CV_THRESH_BINARY);
    out.convertTo(out, CV_8U);
    ABORT_COMPUTATION()

#ifdef DEBUG
    // save mask
    QString fn = source->source.baseName() + "_mask.png";
    cv::imwrite(fn.toLocal8Bit().toStdString(), overallMask);
#endif

    std::vector<std::vector< cv::Point > > contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours( out, contours, hierarchy,
                      CV_RETR_TREE , //CV_RETR_LIST,
                      CV_CHAIN_APPROX_TC89_KCOS //CV_CHAIN_APPROX_NONE
                      );
    ABORT_COMPUTATION()

    if (rectDetection.size() > 0)
    {
        const int start = contours.size() ;
        contours.resize(start + rectDetection.size());
        hierarchy.resize(start + rectDetection.size());
        cv::Vec4i hi;
        hi[0] = hi[1] = hi[2] = hi[3] = -1;
        for (unsigned int i=0; i< rectDetection.size(); i++)
        {
            ABORT_COMPUTATION()
            hierarchy[i+start] = hi;
            contours[i+start].resize(4);
            std::vector<cv::Point2f> pts(4);
            rectDetection[i].size.width /= scale;
            rectDetection[i].size.height /= scale;
            rectDetection[i].center.x /= scale;
            rectDetection[i].center.y /= scale;

            rectDetection[i].points(pts.data());
            for (int j=0; j<4; j++) {
                contours[i+start][j] = pts[j];
            }
        }
    }

    // compute needed data
    QVector<cv::RotatedRect> rects(contours.size());
    QVector<QVector<cv::Point2f> > points(contours.size());
    QVector<bool> keep(contours.size());
    keep.fill(true);
    QVector<double> energies;

    for (int j=0; j< (int)contours.size(); j++) {
        rects[j] = minAreaRect (contours[j]);
        points[j].resize(4);
        rects[j].points(points[j].data());
    }
    ABORT_COMPUTATION()

    PreloadSource::preSelectFast(overallMask, contours, hierarchy, rects, points, keep);
    ABORT_COMPUTATION()

#ifdef DEBUG
    /// Draw contours
    ///
    cv::RNG rng(12345);
      cv::Mat drawing = cv::Mat::zeros( overallMask.size(), CV_8UC3 );
      for( unsigned int i = 0; i< contours.size(); i++ )
         {
            if (hierarchy[i][3] <0) {
                cv::Scalar color = cv::Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                cv::drawContours( drawing, contours, i, color, 2, 8, hierarchy, _maxHierarchyLevel);
            }
         }

    fn = source->source.baseName() + "_conts.png";
    cv::imwrite(fn.toLocal8Bit().toStdString(), drawing);
#endif

    QVector<QVector<float> > sumOfContourLengths(contours.size());
    //computeContourLengths(contours, sumOfContourLengths);

    //removeFilaments(sumOfContourLengths, keep, contours);

    // detect overlapping rectangles and split others, as well
    rectangleOverlap(contours, rects, hierarchy, points, sumOfContourLengths, keep);
    ABORT_COMPUTATION()

    // select best rectangle based on hierachy structure
   //cv::GaussianBlur(overallMask, overallMask, cv::Size(0,0), 3.5);


    // do it again, find contours seems to destroy out
    // cv::threshold(overallMask, out, _threshold*levels, 255, CV_THRESH_BINARY);
    out = overallMask * int(_threshold * float(levels));
    cv::GaussianBlur(out, out, cv::Size(0,0), 5.0);
    out.convertTo(out, CV_8U);

#ifdef DEBUG
    fn = source->source.baseName() + "_blur.png";
    cv::imwrite(fn.toLocal8Bit().toStdString(), out);
#endif
    ABORT_COMPUTATION()


    bestRectangles(im,
                   out,
                   thresholds[bestThresh>=0? bestThresh : 0],
                   contours,
                   hierarchy,
                   rects,
                   points,
                   energies,
                   keep);

    ABORT_COMPUTATION()

#ifdef DEBUG
    /// Draw contours
    ///
      drawing = cv::Mat::zeros( overallMask.size(), CV_8UC3 );
      for( unsigned int i = 0; i< contours.size(); i++ )
         {
            if (hierarchy[i][3] <0) {
                cv::Scalar color = cv::Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                cv::drawContours( drawing, contours, i, color, 2, 8, hierarchy, _maxHierarchyLevel);
            }
         }

    fn = source->source.baseName() + "_conts_noFilaments.png";
    cv::imwrite(fn.toLocal8Bit().toStdString(), drawing);
#endif

    // now extract rectangles and remove remaining overlaps
    // sort by energy. this means a rectangle cannot lose and the winner loses itself.
    QVector<QPair<double, int> > backmap(contours.size());
    for (unsigned int i=0; i<contours.size(); i++)
    {
        backmap[i] = QPair<double, int>(energies[i], i);
    }
    qSort(backmap.begin(), backmap.end(), QPairFirstComparer());

#ifdef DEBUG_TEST
    cv::Mat visualize;
    im.copyTo(visualize);
#endif

    cv::GaussianBlur(overallMask, overallMask, cv::Size(0,0), 5.0);

    for (int j=0; j< (int)contours.size(); j++) {
        ABORT_COMPUTATION()
        const int ind1 = backmap[j].second;
        cv::RotatedRect& rect = rects[ind1];
        if (keep[ind1]) {
            // find overlapping rectangles - disregard the one with less energy
            QPolygonF p;
            p << CvPt2QtPt(points[ind1][0]) << CvPt2QtPt(points[ind1][1])
                    << CvPt2QtPt(points[ind1][2]) << CvPt2QtPt(points[ind1][3]);
            for (int i=j+1; i<(int)contours.size(); i++)
            {
                const int ind2 = backmap[i].second;
                if (keep[ind2])
                {
                    QPolygonF p2;
                    p2 << CvPt2QtPt(points[ind2][0]) << CvPt2QtPt(points[ind2][1])
                            << CvPt2QtPt(points[ind2][2]) << CvPt2QtPt(points[ind2][3]);
                    const QPolygonF intersected = p.intersected(p2);
                    const double area = polyArea(intersected);
                    const double a1 = area / (rects[ind1].size.area());
                    const double a2 = area / (rects[ind2].size.area());
                    if (a1 > _maxOverlap || a2 > _maxOverlap)
                    {
                        keep[ind2] = false;
                    }
                }
            }

#ifdef DEBUG_TEST
            rect.points(points[ind1].data());
            for (int i=0; i<4; i++) {
                cv::line(visualize, points[ind1][i], points[ind1][(i+1)%4], cv::Scalar(0,0,255), 3);
            }
#endif
            // optimize rectangle
            optimizeRectangle(overallMask, rect);
            rect.points(points[ind1].data());
#ifdef DEBUG_TEST
            for (int i=0; i<4; i++) {
                cv::line(visualize, points[ind1][i], points[ind1][(i+1)%4], cv::Scalar(0,255,255), 2);
            }
#endif

            const float scaleNew = scale / source->scale;
            QVector<QPointF> corners(4);
            corners[0] = QPointF(scaleNew*rect.size.width/2.0, scaleNew*rect.size.height/2.0);
            corners[1] = QPointF(scaleNew*rect.size.width/2.0, -scaleNew*rect.size.height/2.0);
            corners[2] = QPointF(-scaleNew*rect.size.width/2.0, -scaleNew*rect.size.height/2.0);
            corners[3] = QPointF(-scaleNew*rect.size.width/2.0, scaleNew*rect.size.height/2.0);
            TargetImagePtr target(new TargetImage);
            target->boundary->setCorners(corners.data());
            target->boundary->setTransform(target->boundary->transform().translate(scaleNew*rect.center.x, scaleNew*rect.center.y).rotate(rect.angle));

            source->targets.push_back(target);
            }
    }

#ifdef DEBUG_TEST
    QString fn2 = QString("%1_%2_%3_%4_"  + source->source.baseName() + "_prodBorder_results.png").arg(_e1).arg(_e2).arg(_e3).arg(_e4);
    cv::imwrite(fn2.toLocal8Bit().toStdString(), visualize);
#endif
}

void PreloadSource::updateMinMax(const int mi, const int ma)
{
    _min = mi;
    _max = ma;
}

void PreloadSource::stop()
{
    _stopped = true;
}

QString PreloadSource::isCurrentlyLoading() const
{
    return _currentFilename;
}

inline bool PreloadSource::testAbort(SourceFilePtr sourceFile)
{

    if (_stopped || _abortAndLoadNext) {
        sourceFile->imageOrig = QImage();
        sourceFile->image.reset();
        preloadMutex.unlock();
        waitFinished.wakeAll();
        _currentFilename = "";
        return true;
    }
    return false;
}

#define TEST_ABORT(s) \
    if (testAbort(s)) { \
        if (locked) CopyTargets::memorySaveMutex.unlock(); \
        locked = false; \
        continue; \
    }

void PreloadSource::run()
{
    while (!_stopped) {
        _abortAndLoadNext = false;
        SourceFilePtr sourceFile;
        int id;
        {
            QMutexLocker l(&_fileListMutex);
            if (_filesToLoad.size() == 0) {
                waitFinished.wakeOne();
                _condition.wait(&_fileListMutex);
                // continue: in the meantime it might have been stopped
                // or set empty
                continue;
            }
            sourceFile = _filesToLoad.first().first;

            _current = sourceFile;
            id = _filesToLoad.first().second;

            _filesToLoad.pop_front();
        }


        preloadMutex.lock();
        _currentFilename = sourceFile->source.absoluteFilePath();
        qDebug() << "starting to load: " << _currentFilename;

        QImage tmpIm = QImage(sourceFile->source.absoluteFilePath());

        CopyTargets::memorySaveMutex.lock();
        bool locked = true;
        sourceFile->error = false;
        sourceFile->imageOrig = tmpIm.convertToFormat(_qImageFmt);
        TEST_ABORT(sourceFile);

        cv::Mat loadedData;
        if (!sourceFile->imageOrig.isNull() || sourceFile->imageOrig.width() == 0)
        {
            // try to load a 16 bit image with opencv
            loadedData = loadAndShrink(sourceFile->source.absoluteFilePath());

            if (loadedData.empty())
            {
                sourceFile->error = true;
            }
            else
            {
                // QImage has different rgb ordering
                cv::Mat tmp;
                cv::cvtColor(loadedData, tmp, CV_BGR2RGB);

                sourceFile->imageOrig = QImage(tmp.data,
                                               tmp.size().width,
                                               tmp.size().height,
                                               tmp.step[0],
                                               QImage::Format_RGB888).convertToFormat(_qImageFmt);
            }
        }
        TEST_ABORT(sourceFile);

        if (!sourceFile->error)
        {
            // scale down original image
            const int maxExtent = 2000;
            sourceFile->scale = 1.0;
            if (sourceFile->imageOrig.width() > maxExtent
                    && sourceFile->imageOrig.width() >= sourceFile->imageOrig.height())
            {
                sourceFile->scale = (double) sourceFile->imageOrig.width() / (double)maxExtent;
                sourceFile->imageOrig = sourceFile->imageOrig.scaled(QSize(maxExtent, maxExtent), Qt::KeepAspectRatio);
            }
            else if (sourceFile->imageOrig.height() > maxExtent
                     && sourceFile->imageOrig.width() < sourceFile->imageOrig.height())
            {
                sourceFile->scale = (double) sourceFile->imageOrig.height() / (double)maxExtent;
                sourceFile->imageOrig = sourceFile->imageOrig.scaled(QSize(maxExtent, maxExtent), Qt::KeepAspectRatio);
            }
            TEST_ABORT(sourceFile);

            if (id >= 0)
            {
                newTargets(sourceFile, loadedData, locked); // if id < 0 then it means only load images
            }
        }
        if (locked) CopyTargets::memorySaveMutex.unlock();
        locked = false;
        TEST_ABORT(sourceFile);

        // remove from list - since we will have it commited in a couple of lines
#ifndef TEST_DEBUG
        if (!_stopped)
        {
            emit doneLoading(sourceFile, id < 0 ? -id -1 : id);
        }
#else
        sourceFile->image = QGraphicsPixmapItemPtr();
        sourceFile->imageOrig = QImage();
        for (int i=0; i<sourceFile.targets.size(); i++) {
            sourceFile->targets[i]->image = QImage();
        }
        sourceFile->targets.clear();
#endif
        _currentFilename = "";
        preloadMutex.unlock();
        waitFinished.wakeAll();
    }
}


/**
 * advanced extracting here
 */

inline cv::Point2f lineIntersection(const cv::Vec2f& l1,
                                    const cv::Vec2f& l2)
{
    const float& theta1 = l1[1];
    const float& rho1   = l1[0];
    const float& theta2 = l2[1];
    const float& rho2   = l2[0];
    const float c1 = qCos(theta1);
    const float c2 = qCos(theta2);
    const float s1 = qSin(theta1);
    const float s2 = qSin(theta2);

    /* cross product l1 x l2 = intersection point (homogeneous)
    [r1*s2 - r2*s1],
    [c1*r2 - c2*r1],
    [c1*s2 - c2*s1]] */

    cv::Point2f out;
    const float third = c1*s2 - c2*s1;

    out.x = (rho1*s2 - rho2*s1)/third;
    out.y = (c1*rho2 - c2*rho1)/third;
    return out;
}

inline std::vector<cv::Point2f> rectanglePoints(const cv::Vec2f& l1,
                                                const cv::Vec2f& l2,
                                                const cv::Vec2f& l3,
                                                const cv::Vec2f& l4)
{
    std::vector<cv::Point2f> out(4);
    out[0] = lineIntersection(l1, l2);
    out[1] = lineIntersection(l2, l3);
    out[2] = lineIntersection(l3, l4);
    out[3] = lineIntersection(l4, l1);
    return out;
}

std::vector<float> lineValues(const cv::Mat& image,
                              const cv::Point2f p1,
                              const cv::Point2f p2)
{
    assert(image.type() == CV_32F);
    cv::Point2f diff = p2 - p1;
    const int count= cv::norm(diff);
    std::vector<float> out(count);
    for (int i=0; i<count; i++) {
        cv::Point2f p(p1.x + diff.x * float(i) / (float)count,
                      p1.y + diff.y * float(i) / (float)count);
        out[i] = image.at<float>((int)p.y, (int)p.x);
    }
    return out;
    /*
    const IplImage tmp = image;
    CvLineIterator iterator;
    const int count = cvInitLineIterator( &tmp, p1, p2, &iterator, connectivity);
    std::vector<float> out(count);
    for( int i = 0; i < count; i++ ){
        out[i] = iterator.ptr[0];
        CV_NEXT_LINE_POINT(iterator);
    }
    return out;*/
}

inline float dot(const cv::Vec2f& v1, const cv::Vec2f v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1];
}


std::vector<cv::RotatedRect> PreloadSource::extractRectangles(const std::vector<cv::Vec3i> &thresholds,
                                                              const cv::Mat& image)
{
    _rectAngularThresh = M_PI * 2.0/180.0;
    _rectMinLineDistFrac = 0.3;
    _maxArea = 0.75;
    const float minLineDist = qMin(image.size().width, image.size().height) * _rectMinLineDistFrac;

    std::vector<cv::Vec2f> lines = extractLines(thresholds, image);

    if (lines.size() >= 150)
    {
        const int n= lines.size();
        std::vector<cv::Vec2f> lines2;
        lines2.reserve(n/150);
        for (unsigned int i=0; i<lines.size(); i+= (int) n /150)
        {
            lines2.push_back(lines[i]);
        }
        lines = lines2;
    }

    _rectMinDist = image.size().width > image.size().height ?
                image.size().width/2000
              : image.size().height/2000 ;
    _rectMinDist = qMax(_rectMinDist, 2.0f);

    // add the boundary lines, since images at the boundary
    // will usually have no lines detected at that boundary
    lines.reserve(lines.size() + 4);
    lines.push_back(cv::Vec2f(0, 0.5 * M_PI));
    lines.push_back(cv::Vec2f(0, 2.0));
    lines.push_back(cv::Vec2f(image.size().width, 0.0));
    lines.push_back(cv::Vec2f(image.size().height, 0.5 * M_PI));
    const int n = lines.size();

    QList<QPair<int, int> > parallelLines;
    QList<float> averageAngle;
#pragma omp parallel for
    for (int i=0; i<n; i++) {
        const float& rho1 = lines[i][0];
        const float& theta1 = lines[i][1];
        for (int j= i+1; j < n; j++) {
            const float& rho2 = lines[j][0];
            const float& theta2 = lines[j][1];
            const float angularDistance = qAbs(theta1 - theta2);
            const float lineDist = qAbs(rho1 - rho2);
            if (angularDistance < _rectAngularThresh
                    && lineDist > minLineDist)
            {
#pragma omp critical
                {
                    parallelLines.append(QPair<int, int>(i,j));
                    averageAngle.append(0.5 * (theta1 + theta2));
                }
            }
        }
    }

        /*
     * look for pairs of parallel lines which meet the rectangle criteria
     */
    const float imageArea = image.size().width * image.size().height;
    std::vector<cv::RotatedRect> out;
    std::vector<std::vector<cv::Point2f> > points;

    QList<QPair<int, int> >::const_iterator it1(parallelLines.begin());
    QList<float>::const_iterator avgAngle1(averageAngle.begin());
    while (it1 != parallelLines.end() && !_stopped && !_abortAndLoadNext)
    {
        const QPair<int, int> parallelLines1 = *it1; it1++;
        const float angle1 = *avgAngle1; avgAngle1++;

        QList<QPair<int, int> >::const_iterator it2(it1);
        QList<float>::const_iterator avgAngle2 (avgAngle1);

        while(it2 != parallelLines.end())
        {
            const QPair<int, int> parallelLines2 = *it2; it2++;
            const float angle2 = *avgAngle2; avgAngle2++;

            // 90 degree
            const float angularDist = qAbs(qAbs(angle1 - angle2) - 0.5 * M_PI);
            if (angularDist < _rectAngularThresh)
            {
                // it is close to a rectangle
                // estimate enclosing rectangle points
                std::vector<cv::Point2f> pts = rectanglePoints(
                            lines[parallelLines1.first],
                        lines[parallelLines2.first],
                        lines[parallelLines1.second],
                        lines[parallelLines2.second]);
                const cv::RotatedRect rect = cv::minAreaRect(pts);
                const float aspect = rect.size.width > rect.size.height ?
                            rect.size.width / rect.size.height
                          : rect.size.height / rect.size.width;
                assert(aspect >= 1.0);
                const  float relArea = rect.size.area() / imageArea;
                if (aspect > _maxAspect || relArea < _minArea || relArea > _maxArea)
                {
                    continue;
                }

                out.push_back(rect);
                points.push_back(pts);
            }
        }
    }

#ifdef DEBUG
    cv::Mat im;
    image.copyTo(im);
    for (unsigned int i=0; i<lines.size(); i++)
    {
        float a = qCos(lines[i][1]);
        float b = qSin(lines[i][1]);
        float x0 = a*lines[i][0];
        float y0 = b*lines[i][0];
        int x1 = int(x0 + 10000*(-b));
        int y1 = int(y0 + 10000*(a));
        int x2 = int(x0 - 10000*(-b));
        int y2 = int(y0 - 10000*(a));
        cv::line( im, cv::Point(x1,y1), cv::Point(x2,y2), cv::Scalar(255,0,0), 2, 8 );
    }

    for (unsigned int i=0; i<out.size(); i++)
    {
        std::vector<cv::Point2f> pts(4);
        out[i].points(pts.data());
        cv::Scalar colorScalar = cv::Scalar( (i*7)%256, 128+ (i*19)%256, 220 + (i*11 )%256);
        for( int j = 0; j < 4; j++ )
           cv::line( im, pts[j], pts[(j+1)%4], colorScalar, 1, 8 );
    }
    cv::imwrite(QString("test_lines_rects%1.png").arg(tmpVal++).toLocal8Bit().toStdString(), im);
#endif

    return out;
}

std::vector<cv::Vec3i> PreloadSource::getThresholds(const cv::Mat& image,
                                                    int& bestThresInd)
{
    // the last one is a threshold from lower
    _histPositions = 3;
    _histWindowWidth = 20;
    _histMinPercentage = 0.1;
    _minColorHeight = 180;


    // Calculate histogram
    std::vector<cv::MatND> hist(3);
    int histSize = 256;    // bin size
    float range[] = { 0, 255 };
    const float *ranges[] = { range };

    std::vector<cv::Mat> channels(3);
    cv::split(image, channels);

    std::vector<cv::Vec3i> out;
    std::vector<float> peakheights;

    for (int pos = 0; pos <_histPositions; pos++)
    {
        cv::Vec3i threshs;
        peakheights.push_back(0);
        for (auto i=0; i<3; i++)
        {
            cv::calcHist( &channels[i], 1, 0, cv::Mat(), hist[i], 1, &histSize, ranges, true, false );

            cv::normalize(hist[i], hist[i], 0, histSize, cv::NORM_MINMAX, -1, cv::Mat() );
            //cv::normalize();

            threshs[i] = findHistPeak(hist[i],
                                           true,
                                           _histWindowWidth,
                                           _histMinPercentage,
                                           pos);
            if (threshs[i] == -1)
            {
                threshs[i] = 0;
                continue;
            }
            peakheights[pos] += hist[i].at<float>(threshs[i]);
        }
        out.push_back(threshs);
    }

    // determine thresh with highest count
    float best=0;
    bestThresInd = -1;
    for (unsigned int i=0; i<peakheights.size(); i++)
    {
        if (peakheights[i] > best
                && out[i][0] > _minColorHeight
                && out[i][1] > _minColorHeight
                && out[i][2] > _minColorHeight)
        {
            best = peakheights[i];
            bestThresInd = i;
        }
    }

    // find from black threshold
    cv::Vec3i thresh;
    for (auto i=0; i<3; i++)
    {
        cv::calcHist( &channels[i], 1, 0, cv::Mat(), hist[i], 1, &histSize, ranges, true, false );

        cv::normalize(hist[i], hist[i], 0, histSize, cv::NORM_MINMAX, -1, cv::Mat() );

        thresh[i] = findHistPeak(hist[i],
                                       false,
                                       _histWindowWidth,
                                       _histMinPercentage);
        if (thresh[i] == -1)
        {
            thresh[i] = 0;
            continue;
        }
    }
    out.push_back(thresh);
    return out;
}

std::vector<cv::Vec2f> PreloadSource::extractLines(const std::vector<cv::Vec3i>& thresholds,
                                                   const cv::Mat& image)
{
    // copy to settings
    _imSize = 500;
    _histSigma = 15;
    _cannyTr1 = 150;
    _cannyTr2 = 300;
    _medianBlurMask = 15;
    _houghRho = 1;
    _houghAngle = 180;
    _houghFactor = 0.25;

    float scale;
    cv::Mat im;
    if (image.size().width < image.size().height)
    {
        scale = (float) image.size().height / (float) _imSize;
        cv::resize(image, im, cv::Size(int(float(image.size().width)/scale),
                                       _imSize));
    }
    else
    {
        scale = (float) image.size().width / (float) _imSize;
        cv::resize(image, im, cv::Size(_imSize,
                                       int(float(image.size().height)/scale)));
    }

    std::vector<cv::Mat> channels(3);
    cv::split(im, channels);

    // first canny step
    cv::Mat canny;
    cv::cvtColor(im, canny, cv::COLOR_BGR2GRAY);
    cv::Canny(canny, canny, _cannyTr1, _cannyTr2);

    /*
     * here, different thresholds are tried (meaning that these thresholds are assumed
     * as white scanner background) -> more canny steps
     */

    cv::Mat mask = cv::Mat::zeros(im.size(), CV_8U);
    for (int pos = 0; pos <_histPositions; pos++)
    {
        // threshold over different histogram peaks (aka pos)
        for (auto i=0; i<3; i++)
        {

            cv::Mat dst(im.size(), CV_8U);
            cv::threshold(channels[i], dst, thresholds[pos][i] - _histSigma, 1, cv::THRESH_BINARY_INV);
            mask = cv::max(mask, dst);
            dst.release();
        }

        cv::medianBlur(mask, mask, _medianBlurMask);
        // apply threshold
        std::vector<cv::Mat> newChannels(3);
        for (int i=0; i<3; i++)
        {
            cv::multiply(mask, channels[i], newChannels[i]);
        }

        cv::Mat tmp;
        cv::merge(newChannels, tmp);
        cv::cvtColor(tmp, tmp, cv::COLOR_BGR2GRAY);
        cv::Mat cannyTmp;
        cv::Canny(tmp, cannyTmp, _cannyTr1, _cannyTr2);
        canny = cv::max(canny, cannyTmp);
    }

    /*
     * now black scanner background
     */
    {
        mask = cv::Mat::zeros(im.size(), CV_8U);
        for (auto i=0; i<3; i++)
        {
            cv::Mat dst;
            cv::threshold(channels[i], dst, thresholds[_histPositions][i] + _histSigma, 255, cv::THRESH_BINARY_INV);
            //
            mask = cv::max(mask, dst);
            dst.release();
        }

        cv::medianBlur(mask, mask, _medianBlurMask);
        // apply threshold
        std::vector<cv::Mat> newChannels(3);
        for (int i=0; i<3; i++)
        {
            newChannels[i] = cv::max(mask, channels[i]);
        }
        cv::Mat tmp;
        cv::merge(newChannels, tmp);
        cv::cvtColor(tmp, tmp, cv::COLOR_BGR2GRAY);
        cv::Mat cannyTmp;
        cv::Canny(tmp, cannyTmp, _cannyTr1, _cannyTr2);
        canny = cv::max(canny, cannyTmp );
    }

    /* continue with hough transform */
    //cv::medianBlur(canny, canny, 3); //_medianBlurMaskOverall);
#ifdef DEBUG
    cv::imwrite(QString("test_canny%1.png").arg(tmpVal).toLocal8Bit().toStdString(), canny);
#endif

    std::vector<cv::Vec2f> lines;
    cv::HoughLines(canny, lines, _houghRho, M_PI / float(_houghAngle), int(_houghFactor*_imSize));

    for (unsigned int i=0; i<lines.size(); i++)
    {
        lines[i][0] *= scale;
    }

    return lines;
}


int PreloadSource::findHistPeak(const cv::Mat& hist,
                                const bool fromRight,
                                const int windowWidth,
                                const float minPercentage,
                                const int position) const
        //hist, fromRight, width = 10, minVal = 0.3, pos = 0):
{
    const int n = hist.size[0];
    int found = 0;
    for (int i = (fromRight ? n-1 : 0); i>=0 && i< n; (fromRight? i-- : i++))
    {
        bool higher = true;
        for (int j=1; j<windowWidth; j++)
        {
            if (hist.at<float>(i)  < hist.at<float>(qMin(i+j, n-1))
                    || hist.at<float>(i)  < hist.at<float>(qMax(i-j, 0))
                    || hist.at<float>(i) < 256.0f * minPercentage)
            {
                higher = false;
                break;
            }
        }
        if (higher)
        {
            // retrieve pos'th peak
            if (found == position)
            {
                return i;
            }
            found ++;
        }
    }
    return -1;
}



inline cv::RotatedRect rectFromData(const lbfgsfloatval_t *x)
{
    cv::RotatedRect rectangle;
    rectangle.angle = x[0];
    rectangle.center.x = x[1];
    rectangle.center.y = x[2];
    rectangle.size.width = x[3];
    rectangle.size.height = x[4];
    return rectangle;
}

cv::Mat PreloadSource::_currentImageForRectOpt;

lbfgsfloatval_t PreloadSource::evaluate(
    void *instance,
    const lbfgsfloatval_t *x,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t stepOrig
    )
{
    const cv::Mat& image = _currentImageForRectOpt;
    cv::RotatedRect rectangle = rectFromData(x);
    lbfgsfloatval_t fx = 0.0;
    QVector<cv::Point2f> pts(4);
    double sumValues;
    long int numPixels;

    const lbfgsfloatval_t step = stepOrig <= 1e-10 ? 1.0 : stepOrig;

    rectangle.points(pts.data());
    getSumOfRectangleSamplingF(pts, image, sumValues, numPixels);
    fx  = -(lbfgsfloatval_t) ( sumValues / (double) numPixels);

    // compute gradient
    for (int i=0; i<n; i++) {
        lbfgsfloatval_t x2[5];
        for (int j=0; j < n; j++) {
            //x2[j] = j==i ? x[j] + steps[j] : x[j];
            x2[j] = j==i ? x[j] + step : x[j];
        }

        rectangle = rectFromData(x2);
        rectangle.points(pts.data());
        getSumOfRectangleSamplingF(pts, image, sumValues, numPixels);
        lbfgsfloatval_t fx2  = -(lbfgsfloatval_t) ( sumValues / (double) numPixels);

        g[i] = (fx2 - fx) / (step );
    }
    return fx;
}

void PreloadSource::optimizeRectangle(const cv::Mat& edgeMask,
        cv::RotatedRect &rectangle)
{
    edgeMask.copyTo(_currentImageForRectOpt);
    int ret = 0;
    const int N = 5;
    lbfgsfloatval_t fx;
    lbfgsfloatval_t *x = lbfgs_malloc(N);
    lbfgs_parameter_t param;
    if (x == NULL) {
        qCritical() << "ERROR: Failed to allocate a memory block for variables.";
        return;
    }
    /* Initialize the variables. */
    x[0] = rectangle.angle;
    x[1] = rectangle.center.x;
    x[2] = rectangle.center.y;
    x[3] = rectangle.size.width;
    x[4] = rectangle.size.height;

    //qDebug() << "before: " << x[0] << x[1] << x[2] << x[3] << x[4];

    /* Initialize the parameters for the L-BFGS optimization. */
    lbfgs_parameter_init(&param);
    /*param.linesearch = LBFGS_LINESEARCH_BACKTRACKING;*/

    param.linesearch = LBFGS_LINESEARCH_BACKTRACKING ;//LBFGS_LINESEARCH_BACKTRACKING_ARMIJO;

    /*
        Start the L-BFGS optimization; this will invoke the callback functions
        evaluate() and progress() when necessary.
     */
    ret = lbfgs(N, x, &fx, evaluate, NULL, NULL, &param);
    Q_UNUSED(ret);
    /* Report the result. */
    //qDebug() << "after: " << x[0] << x[1] << x[2] << x[3] << x[4];
    //qDebug() << "L-BFGS optimization terminated with status code =" << ret;
    rectangle = rectFromData(x);

    lbfgs_free(x);
    return;
}
