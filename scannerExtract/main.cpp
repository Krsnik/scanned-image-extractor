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

#include <QtGui>

#include <iostream>
#include "mainwindow.h"

#include "TargetImage.h"
#include "sourcefile.h"
#include "translation.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<Rotation90>("Rotation90");
    qRegisterMetaType<SourceFile>("SourceFile");

    QApplication app(argc, argv);
    QStringList args = app.arguments();

    // ### load locale
    QString locale = QLocale::system().name();
    for (int i=args.length()-1; i>0; i--) {
        if (args[i]== "-l" || args[i] == "--locale") {
            if (argc <= i+1) {
                qWarning() << QString("please provide a locale, e.g.: %1 -l en").arg(args[0]);

            } else {
                locale = args[i+1];
                qDebug() << "trying to load locale " << locale << "...";
                args.removeAt(i+1);
            }
            args.removeAt(i);
        }
    }

    // set locale and load translations
    MiscTools::Translation::setDefaultLocale(locale);

    QStringList langPaths = QStringList() << QString(":/");
    app.installTranslator(MiscTools::Translation::getTranslator(locale, langPaths, "qt_"));
    app.installTranslator(MiscTools::Translation::getTranslator(locale, langPaths, "qtbase_"));
    app.installTranslator(MiscTools::Translation::getTranslator(locale, langPaths, "qt_help_"));
    app.installTranslator(MiscTools::Translation::getTranslator(locale, langPaths, "trans_scannedImageExtractor_"));

    // ### setup warranty display
    QFile file(QApplication::tr(":/WARRANTY_EN", "start"));
    std::cout << "This is Scanned Image Extractor - version "
              << version_scannerExtract.getVersionMajor() << "."
              << version_scannerExtract.getVersionMinor() << "."
              << version_scannerExtract.getVersionPatch()
              << std::endl;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd()) {
            std::cout << in.readLine().toStdString() << std::endl;
        }
        std::cout << std::endl;
    } else {
        std::cout << "not found" << std::endl;
    }
    file.close();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
