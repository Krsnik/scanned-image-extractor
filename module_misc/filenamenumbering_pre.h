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
 * Copyright (C) 2015, Dominik Rueß; info@dominik-ruess.de
 **********************************************************************/

#ifndef FILENAMENUMBERING_PRE_H
#define FILENAMENUMBERING_PRE_H

#include <QVector>
#include <QVariant>
#include <QDateTime>
#include <QStringList>

namespace MiscTools
{

/**
  * use this class to avoid unintended use of direct overloaded
  * operator== in QList for template QVariant
  */
template <typename T>
class MyFilePartsList : public QVector<T>
{
public:
    template<typename T2>
    Q_OUTOFLINE_TEMPLATE bool operator==(const MyFilePartsList<T2> &b) const;

};

class FilenameNumberingTests;

typedef MyFilePartsList<QVariant> InfoList;

class FilenameNumbering_PRE
{
    friend class FilenameNumberingTests;

private:

    FilenameNumbering_PRE();
public:

    typedef QPair<QDateTime, bool> DT;

    static InfoList splitFileToStringParts(const QString& filename,
            const bool caseInsensitive = false,
            const bool withoutExtension = true,
                                           InfoList * stringValues = NULL);

    static QVector<InfoList> getDifferentFile_Patterns(
            QStringList filenames,
            const bool caseInsensitive = false,
            const bool withoutExtension = true);

    static void getUniqueNumbersAndValues(const QStringList filenames,
                                          const bool diffSuffixSameNumbers,
                                          QVector<QVector<int> >& fileNumberValues,
                                          QVector<bool>& numbersUnique,
                                          QVector<int>& numberingPositions,
                                          bool &onePatternFound,
                                          const bool caseInsensitive = false,
                                          const bool withoutExtension = true);

    static bool getTagsBackMapAndNumbers(
        const QStringList& filenames,
        const QString& taggedFileName,
        const QVector<QPair<QString, QString> >& tagsAndDateMap,
        const QString& tag,
        QVector<int >& runningFileNumbers,
        QVector<QVector<QPair<int, int> > >& startAndLengthOfRunnNumbers);
    

    /**
     * there might be a non direct renaming graph, try to find it
     */
    static QVector<int> findMoveOrdering(
            const QList<QPair<QString, QString> > &moveExisting
            );    

protected:
    static bool _getTagMap(
            const QStringList &filenames,
            const QString &taggedFileName,
            const QVector<QPair<QString, QString> > &tagsAndDateMap, // first = tagNo, second = tagOrderPos;
            QMultiMap<int, int> &tagPositions,
            QVector<int> &datePatternsUsed,
            QString &datePattern,
            QVector<QVector<QVariant> > &filenameNumericValues,
            QVector<QVector<QPair<int, int> > > &tagSourceFNPositionsAndLengths);
};

} // namespace MiscTools

#endif // FILENAMENUMBERING_PRE_H
