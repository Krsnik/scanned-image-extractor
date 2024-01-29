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
 * Copyright (C) 2013, Dominik Rueß; info@dominik-ruess.de
 **********************************************************************/

#ifndef VERSIONING_H
#define VERSIONING_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#ifdef QT_NETWORK_LIB
#include <QDataStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#endif


// determine bits of this compiler
// http://stackoverflow.com/a/1505664
template<int> static int DetermineBitsHelper();
template<> int DetermineBitsHelper<4>() {
  return 32;
}
template<> int DetermineBitsHelper<8>() {
  return 64;
}
// helper function just to hide clumsy syntax
 static int DetermineBits() { return DetermineBitsHelper<sizeof(size_t)>(); }

/**
 * class to allow for checking version requirements
 *
 * APR's Version Numbering:
 * http://apr.apache.org/versioning.html
 */
template<int major, int minor, int patch>
struct VersionNumber
{
    VersionNumber()
    {
        checkRequirements();
    }

    template<int requireMajor, int usedMinor>
    void checkCompatibility() const
    {
        static_assert(requireMajor == major, "Incompatible major library version");
        static_assert(usedMinor <= minor, "Incompatible minor version: library too old");
    }

    virtual bool checkForUpdateVersion(const QString& location,
                                       const qint32 magicNumber,
                                       qint32& majorRemote,
                                       qint32& minorRemote,
                                       qint32& patchRemote,
                                       QString& link,
                                       QString& text,
                                       bool& connection) const
    {
#ifdef QT_NETWORK_LIB
        QNetworkAccessManager* m_NetworkMngr = new QNetworkAccessManager();
        QNetworkReply *reply= m_NetworkMngr->get(QNetworkRequest(location));
        QEventLoop loop;
        loop.connect(reply, SIGNAL(finished()),SLOT(quit()));
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) {
            connection= true;

            QDataStream in(reply);
            qint32 magic;
            quint16 streamVersion;
            in >> magic >> streamVersion;
            if (magic != magicNumber) {
                qCritical() << "Version file is not recognized by this application";
                connection = false;
                return false;
            } else if (streamVersion > in.version()) {
                qCritical() << "Version file is from a more recent version of the application";
                connection = false;
                return false;
            }
            in.setVersion(streamVersion);
            in >> majorRemote >> minorRemote >> patchRemote >> link >> text;
        } else {
            qCritical() << "Could not establish connection " << reply->error() << ":" << reply->errorString();
            connection = false;
            delete m_NetworkMngr;
            return false;
        }

        delete reply;
        delete m_NetworkMngr;

        const bool newVersion =
                major < majorRemote || minor < minorRemote || patch < patchRemote;

        return newVersion;
#else
		Q_UNUSED(location);
		Q_UNUSED(magicNumber);
		Q_UNUSED(majorRemote);
		Q_UNUSED(minorRemote);
		Q_UNUSED(patchRemote);
		Q_UNUSED(link);
		Q_UNUSED(text);
        qCritical() << QString("network library of Qt not linked against application");
        connection = false;
        return false;
#endif
    }

    int getVersionMajor() const
    {
        return major;
    }

    int getVersionMinor() const
    {
        return minor;
    }

    int getVersionPatch() const
    {
        return patch;
    }

    virtual void checkRequirements() const
    {
        static_assert(true, "no requirements needed");
    }

    int getCompilerBits()
    {
        // appropriate function will be selected at compile time
        return DetermineBits();
    }

};

#endif // VERSIONING_H
