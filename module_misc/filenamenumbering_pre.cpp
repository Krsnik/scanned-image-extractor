/***********************************************************************
 * This file is part of module_misc.
 *
 * module_misc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * module_misc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with module_misc.  If not, see <http://www.gnu.org/licenses/>
 * 
 * 
 * Copyright (C) 2015, Dominik Rueß; info@dominik-ruess.de
 **********************************************************************/

#include <QFileInfo>
#include <QDebug>
#include <QMultiMap>
#include <QVector>
#include <QPair>

#include "filenamenumbering_pre.h"

namespace MiscTools
{

/**
  * a separator to build up a filenamestring
  */
#ifdef _WIN32
    #define SEPARATOR QString("<")
#else
    #define SEPARATOR QString('/')
#endif

FilenameNumbering_PRE::FilenameNumbering_PRE()
{
}

InfoList FilenameNumbering_PRE::splitFileToStringParts(const QString& filenameOrig,
                                                   const bool caseInsensitive,
                                                   const bool withoutExtension,
                                                   InfoList *stringValues)
{
    InfoList out;
    if (stringValues != NULL) {
        stringValues->clear();
    }

    const QFileInfo f = filenameOrig;
    QString filename;
    if (withoutExtension) {
        filename = caseInsensitive ? f.completeBaseName().toLower() : f.completeBaseName();
    } else {
        filename = caseInsensitive ? f.fileName().toLower() : f.fileName();
    }

    for (int i=0; i<filename.length(); i++) {
        // add the case that a negative number is present,
        // hence if a negative sign is followed by a digit
        bool negativeSignTest = (filename[i]=='-'
                                 && i < filename.length()-1
                                 && filename[i+1].isDigit()
                                 && (i==0 || filename[i-1] == QChar(' ')) );
        if (filename[i].isDigit() || negativeSignTest) {
            QString num;
            while ((filename[i].isDigit() || negativeSignTest)&& i<filename.length()) {
                num += filename[i];
                i++;
                negativeSignTest=false;//only allowed at first char
            }
            i--;
            out << num.toInt();
            if (stringValues != NULL) {
                (*stringValues) << num;
            }
        } else {
            QString part = "";
            while (!negativeSignTest && !filename[i].isDigit() && i<filename.length()) {
                part += filename[i];
                i++;
                // if a minus character occurs with a following digit, then this is part of a number
                negativeSignTest = (filename[i]=='-' && i < filename.length()-1 && filename[i+1].isDigit());
            }
            i--;
            out << part;
            if (stringValues != NULL) {
                (*stringValues) <<part;
            }
        }
    }
    return out;
}

QVector<InfoList> FilenameNumbering_PRE::getDifferentFile_Patterns(QStringList filenames,
                                                               const bool caseInsensitive,
                                                               const bool withoutExtension)
{
    QVector<InfoList> patterns;
    for (int i=0; i<filenames.size(); i++) {
        const InfoList fileParts = splitFileToStringParts(filenames[i],
                                                caseInsensitive,
                                                withoutExtension);
        if (!patterns.contains(fileParts)) {
            patterns.push_back(fileParts);
        }
    }

    return patterns;
}


void FilenameNumbering_PRE::getUniqueNumbersAndValues(const QStringList filenames,
                                                      const bool diffSuffixSameNumbers,
                                                      QVector<QVector<int> > &fileNumberValues,
                                                      QVector<bool> &numbersUnique,
                                                      QVector<int> &numberingPositions,
                                                      bool & onePatternFound,
                                                      const bool caseInsensitive,
                                                      const bool withoutExtension)
{
    const int nF = filenames.size();
    fileNumberValues.clear();
    numbersUnique.clear();
    numberingPositions.clear();

    const QVector<InfoList> diffPatterns = getDifferentFile_Patterns(filenames, caseInsensitive, withoutExtension);
    if (diffPatterns.size() != 1) {
        onePatternFound = false;
        qWarning() << "different file patterns found, please revise";
        return;
    }
    onePatternFound = true;

    const InfoList patterns = diffPatterns.first();    

    numberingPositions.clear();
    for (int i=0; i<patterns.size(); i++) {
        if (patterns[i].typeName() == QString("int")) {
            numberingPositions.push_back(i);
        }
    }
    const int n = numberingPositions.size();

    if (n == 0) {
        qWarning() << "no numbers found";
        return;
    }

    fileNumberValues.resize(n);
    numbersUnique.resize(n);
    for (int numPos=0; numPos<n; numPos++) {
        fileNumberValues[numPos].resize(nF);
        for (int j=0; j<nF; j++) {
            InfoList f = splitFileToStringParts(filenames[j], caseInsensitive, withoutExtension);
            fileNumberValues[numPos][j] = f.at(numberingPositions[numPos]).toInt();
        }

        // unique test
        bool unique = true;
        QMultiMap<int, int> tmp;
        for (int i=0; i<nF; i++) {
            tmp.insertMulti(fileNumberValues[numPos][i], i);
        }
        QList<int> sortedInd = tmp.values();
        for (int j=0; j<nF; j++) {
            int k = j+1;
            while(k < nF && fileNumberValues[numPos][sortedInd[j]] ==
                  fileNumberValues[numPos][sortedInd[k]])
            {
                if (fileNumberValues[numPos][sortedInd[k]] ==
                        fileNumberValues[numPos][sortedInd[j]]){

                    if (diffSuffixSameNumbers) {
                        const QFileInfo f1 = filenames[sortedInd[k]];
                        const QFileInfo f2 = filenames[sortedInd[j]];
                        if ( (caseInsensitive && f1.suffix().toLower() == f2.suffix().toLower() )
                             || (!caseInsensitive && f1.suffix() == f2.suffix() ) ){
                            unique = false;
                            break;
                        }
                    } else {
                        unique = false;
                        break;
                    }
                }
                k++;
            }
            if (!unique) {
                break;
            }
        }
        numbersUnique[numPos] = unique;
    }
}

bool FilenameNumbering_PRE::getTagsBackMapAndNumbers(
        const QStringList& filenames,
        const QString& taggedFileName,
        const QVector<QPair<QString, QString> >& tagsAndDateMap,
        const QString& tag,
        QVector<int >& runningFileNumbers,
        QVector<QVector<QPair<int, int> > >& startAndLengthOfRunnNumbers)
{
    startAndLengthOfRunnNumbers.clear();
    runningFileNumbers.clear();

    QMultiMap<int, int> tagPositions;
    QVector<QVector<QVariant> > filenameNumericValues;
    QVector<int> tmp1;
    QString tmp2;
    QVector<QVector<QPair<int, int> > > startAndLengthOfAllTags;
    _getTagMap(filenames,
               taggedFileName,
               tagsAndDateMap,
               tagPositions,
               tmp1, tmp2,
               filenameNumericValues,
               startAndLengthOfAllTags);
    if (filenameNumericValues.size() != filenames.size()) {
        qWarning() << "number of filenames mismatch";
        return false;
    }

    // create tag number map to numerical value
    // position in filename,
    QVector<int> numberingPos;
    QMapIterator<int, int> i(tagPositions);
    int k=0;
    while (i.hasNext()) {
        i.next();
        const int tagNo = i.value();
        if (tagsAndDateMap[tagNo].first == tag) {
            numberingPos.push_back(k);
        }
        k++;
    }

    if (numberingPos.size() == 0) {
        qWarning() << "could not determine running number";
        return false;
    }

    const int n = filenames.size();
    runningFileNumbers.resize(n);
    startAndLengthOfRunnNumbers.resize(n);
    for (int j=0; j<n; j++) {
        if (filenameNumericValues[j].size() == 0) {
            qWarning() << "no running number detected for " << filenames.at(j);
            runningFileNumbers[j] = -1;
            continue;
        }
        //startAndLengthOfRunnNumbers[j].resize(k);
        for (int k=0; k<numberingPos.size(); k++) {
            if (filenameNumericValues[j][numberingPos[k]].type() != QVariant::Int) {
                qWarning() << "tried to convert a string to a running number, return false";
                return false;
            }
            if (k==0) {
                runningFileNumbers[j] = filenameNumericValues[j][numberingPos[k]].toInt();
            } else if (filenameNumericValues[j][numberingPos[k]].toInt() != runningFileNumbers[j]){
                qWarning() << "warning, inconsistent running numbers for "
                         << filenames.at(j);
            }
            startAndLengthOfRunnNumbers[j].push_back(startAndLengthOfAllTags[j][numberingPos[k]]);
        }
    }
    return true;
}


bool FilenameNumbering_PRE::_getTagMap(
        const QStringList& filenames,
        const QString& taggedFileName,
        const QVector<QPair<QString, QString> >& tagsAndDateMap,
        QMultiMap<int, int>& tagPositions, // first = tagNo, second = tagOrderPos;
        QVector<int>& datePatternsUsed,
        QString& datePattern,
        QVector<QVector<QVariant> >& filenameNumericValues,
        QVector<QVector<QPair<int, int> > >& tagSourceFNPositionsAndLengths)
{
    // find positions and order of given tags
    datePattern="";
    tagPositions.clear();
    datePatternsUsed.clear();
    filenameNumericValues.clear();
    tagSourceFNPositionsAndLengths.clear();
    for (int i=0;i<tagsAndDateMap.size(); i++) {
        int pos = taggedFileName.indexOf(tagsAndDateMap[i].first);
        while (pos >=0) {
            tagPositions.insertMulti(pos, i);
            pos = taggedFileName.indexOf(tagsAndDateMap[i].first, pos+1);
        }
    }

    // now extract fixed text between tags
    // also create datePatterns/parse format for comparison
    QStringList textInBetween;
    datePattern = "";
    datePatternsUsed.clear();
    QMapIterator<int, int> i(tagPositions);
    int pos = 0;
    int k=0;
    while (i.hasNext()) {
        i.next();
        const int tagPos = i.key();
        const int tagNo = i.value();
        if (tagsAndDateMap[tagNo].second.length() > 0) {
            datePatternsUsed.push_back(k);
            datePattern += QString("%1%2").arg(SEPARATOR).arg(tagsAndDateMap[tagNo].second);
        }
        if (tagPos != pos) {
            textInBetween.push_back(taggedFileName.mid(pos, tagPos-pos));
        }
        pos = tagPos + tagsAndDateMap[tagNo].first.length();
        k++;
        if (!i.hasNext()) {
            if (pos != taggedFileName.length()) {
                textInBetween.push_back(taggedFileName.right(taggedFileName.length()-pos));
            }
        }
    }

    // strip all known text to obtain numerical values
    const int n = filenames.size();
    const int d = tagPositions.size();
    filenameNumericValues.resize(n);
    tagSourceFNPositionsAndLengths.resize(n);
    for (int i=0; i<n; i++) {
        filenameNumericValues[i].resize(d);
        tagSourceFNPositionsAndLengths[i].resize(d);
        QString fn = QFileInfo(filenames[i]).baseName();
        const int originalFilenameLength = fn.length();
        // first of all replace all fixed text with spaces
        int pos = 0;
        int currPosValue = 0;
        int ind=0;
        for (int j=0; j<textInBetween.size(); j++) {            
            int currLength = pos;
            pos = fn.indexOf(textInBetween[j], pos, Qt::CaseInsensitive);
            fn = fn.replace(pos, textInBetween[j].length(), SEPARATOR);
            if (pos == 0) {
                currPosValue += textInBetween[j].length();
            } else if (pos >0) {
                currLength = pos - currLength;
                tagSourceFNPositionsAndLengths[i][ind] = QPair<int, int>(currPosValue, currLength);
                currPosValue += textInBetween[j].length() + currLength;
                ind++;
            }
            // make sure, just inserted space is not considered again:
            pos++;
        }
        if (ind == d-1) {
            // last part of basename was a number
            tagSourceFNPositionsAndLengths[i][ind] = QPair<int, int>(currPosValue, originalFilenameLength - currPosValue);
            ind++;
        } else if (ind != d) {
            qWarning() << "error, problems with determining the positions and lengths of the "
                        " tag values for " << filenames[i] << " " << ind << " " << n-1;
            return false;
        }

        QStringList values = fn.split(SEPARATOR, QString::SkipEmptyParts);
        if (values.length() != d) {
            qWarning() << "mismatching number of tags and found values";
            filenameNumericValues[i].clear();
            continue;
        }
        for (int j=0; j < d; j++) {
            bool ok;
            const int val = values[j].toInt(&ok);
            filenameNumericValues[i][j] = ok ? val : QVariant(values[j]);
        }
    }

    datePattern = datePattern.replace(SEPARATOR, " ");
    return true;
}

QVector<int> FilenameNumbering_PRE::findMoveOrdering(
        const QList<QPair<QString, QString> >& moveExisting
        )
{
    QVector<QString> currentExisting(moveExisting.size());

    QList<QPair<QString, QString> > notHandled = moveExisting;
    QVector<int> backMap(notHandled.size());
    for (int i=0; i<notHandled.size(); i++) {
        backMap[i] = i;
        currentExisting[i] = moveExisting[i].first;
    }

    QVector<int> order;
    int lastSize = order.size() - 1;
    while (notHandled.size() > 0) {
        if (lastSize == order.size()) {
            qCritical() << "could not resolve movement, circular dependencies ?!";
            return QVector<int>();
        }

        lastSize = order.size();

        for (int i=notHandled.size()-1; i>=0; i--) {
            const QString src = notHandled[i].first;
            const QString target = notHandled[i].second;
            // either the file does not exist or it exists but will be removed before
            // current file gets moved
            if (!currentExisting.contains(target) ) {
                order.push_back(backMap[i]);
                //added.push_back(notHandled[i].second);
                //deleted.push_back(notHandled[i].first);
                currentExisting.append(target);
                if (currentExisting.contains(src)) {
                    currentExisting.remove(currentExisting.indexOf(src));
                }
                notHandled.removeAt(i);
                backMap.remove(i);
            }
        }
    }
    return order;
}


template <>
template <>
Q_OUTOFLINE_TEMPLATE bool MyFilePartsList<QVariant>::operator==(const MyFilePartsList<QVariant> &b) const
{
    if (size() != b.size()) {
        return false;
    }
    bool same = true;
    for (int i=0; i < (int)size(); i++) {
        if (this->at(i).typeName() == b.at(i).typeName()) {
            if (this->at(i).typeName() == QString("QString")) {
                same &= this->at(i).toString() == b[i].toString();
            }
        } else {
            same = false;
            break;
        }
    }
    return same;
}

} // end namespace MiscTools
