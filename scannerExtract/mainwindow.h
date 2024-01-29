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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGraphicsScene>
#include <QApplication>

#include "imagescene.h"
#include "about.h"
#include <version_scannerExtract.h>
#include "settingsdialog.h"
#include "helpdialog.h"

#define IMAGE_BACKGROUND_COLOR QColor(220,220,220)

#define WAIT_FOR_DONATE_HINT_MS 30000
//2*60*1000
#define WAIT_FOR_CHECK_FOR_UPDATE 1*60*1000
#define DAYS_UNTIL_DONATE_HINT -5

#define NEW_VERSION_LOCATION_SCANNER_EXTRACT "http://dominik-ruess.de/scannerExtract/currentVersion"
#define NEW_VERSION_MAGIC_NUMBER 29384730

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void newOrientation(const Rotation90 rot);

    void newAspectRatio(const double ratio);

    void newList(QFileInfoList, int);

private slots:
    void _onLoadFile();

    void _onNewTargetImage(const QPixmap& pixmap);

    void _onRotation90(const Rotation90 rotation);

    void on_radioButton_rot0_toggled(bool isChecked);

    void on_radioButton_rot90_toggled(bool isChecked);

    void on_radioButton_rot180_toggled(bool isChecked);

    void on_radioButton_rot270_toggled(bool isChecked);

    void on_toolButton_deleteItem_clicked();

    void on_toolButton_prevItem_clicked();

    void on_toolButton_nextItem_clicked();

    void _updateTargetView(int x=0, int y=0);

    void _getAspectRatio();

    void on_toolButton_zoomIn_clicked();

    void on_toolButton_zoomOut_clicked();

    void on_toolButton_zoomFit_clicked();

    void on_toolButton_zoomOriginal_clicked();

    void on_toolButton_firstImage_clicked();

    void on_toolButton_lastImage_clicked();

    void on_toolButton_nextImage_clicked();

    void on_toolButton_prevImage_clicked();

    void on_toolButton_saveTargtes_clicked();

    void _resetAspect();

    void on_actionAbout_Qt_triggered();

    void on_action_About_triggered();

    void on_actionSupport_by_donation_triggered();

    void on_actionCheck_for_new_version_triggered();

    void provideInfo(const QString& headline,
                                const QString& text,
                                const QString& settingsValue,
                                const QString& showAgain = qApp->translate("tools", "Show hint in future"),
                                const int width = 800,
                                const int minWidth = 600);

    bool lockDialog();

    void unlockDialog();

    void resizeEvent(QResizeEvent *);

    void currImageHasChanges(bool hasChanges);

    void newFileName(const QString);

    void enableButtons();

    void toggleAspect(const float aspect);

    void loadSettings();
    void newValues();

    void onNewCrop(const double crop);

    void on_action_Settings_triggered();

    void selectAll() { _scene.selectAll(); }

    void noRotation();
    void noAspect();
    void noCrop();

    void setWaitTarget(const bool wait);

    void on_actionAbout_liblbfgs_triggered();

    void on_doubleSpinBox_cropInitial_valueChanged(const double val);

    void numSelectionChanged(const int num);

    void on_actionOnline_Help_triggered();

    QString getHelpText();

    void showStartupHelp();

    void on_toolButton_Help_clicked();

    void on_actionAbout_Open_CV_triggered();

    void doneCopy(const QString &filename, const QString &targetLocation);
private:
    Ui::MainWindow *ui;

    ImageScene _scene;

    QGraphicsScene _sceneTarget;

    DialogAbout _about;

    bool _dialogLock;

    QString _title;

    SettingsDialog _settings;

    QProgressBar* _targetWait;

    bool _tearDown;

    HelpDialog _helpDialog;
};

#endif // MAINWINDOW_H
