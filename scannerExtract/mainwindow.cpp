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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDate>
#include <QDateTime>
#include <QWhatsThis>
#include <QPolygonF>
#include <QtWidgets/QFileDialog>
#include <QSettings>
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTextBrowser>
#include <QCheckBox>
#include <QPushButton>
#include <QShortcut>

#include "pQPixmap.h"

#include "imageboundary.h"
#include "TargetImage.h"

#include "settings.h"

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _dialogLock(false),
    _title(tr("Scanned Image Extractor")),
    _targetWait(0),
    _tearDown(false)
{
    ui->setupUi(this);
    qRegisterMetaType<QFileInfoList>("QFileInfoList");
    qRegisterMetaType<SourceFilePtr>("SourceFilePtr");
    qRegisterMetaType<TargetImagePtr>("TargetImagePtr");

    ui->graphicsView_Source->setScene(&_scene);
    _scene.init();
    connect(&_scene, SIGNAL(doneCopying(QString,QString)), SLOT(doneCopy(QString,QString)));

    setWindowTitle(_title);

    connect(ui->actionLoad_Directory, SIGNAL(triggered()),
            this, SLOT(_onLoadFile()));
    connect(&_scene, SIGNAL(changed(bool)), this, SLOT(currImageHasChanges(bool)),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(selectedAspect(float)), this, SLOT(toggleAspect(float)),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(numSelected(int)), this, SLOT(numSelectionChanged(int)),  Qt::QueuedConnection);
    connect(this, SIGNAL(newList(QFileInfoList,int)),
            &_scene, SLOT(newImageList(QFileInfoList,int)), Qt::QueuedConnection);

    statusBar()->showMessage(tr("Hover the mouse pointer over an element to get help"));


    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));
    restoreGeometry(settings.value("mainwin_geom").toByteArray());
    ui->splitter->restoreState(settings.value("mainwin_splitter").toByteArray());
    ui->doubleSpinBox_cropInitial->setValue(settings.value("cropdistance", 2.0).toDouble());
    ui->doubleSpinBox_crop->setValue(settings.value("cropdistance", 3.0).toDouble());
    ui->lineEdit_aspectRatio->setText(settings.value("manualaspect", "4:3").toString());
    ui->graphicsView_Target->setScene(&_sceneTarget);

    connect(ui->checkBox_enforceAspect, SIGNAL(toggled(bool)), &_scene, SLOT(setEnforceAspect(bool)), Qt::QueuedConnection);

    ui->checkBox_enforceAspect->setChecked(settings.value("enforceAspectNextIm", true).toBool());


    connect(&_scene, SIGNAL(updateTargetDisplay(QPixmap)),
            this, SLOT(_onNewTargetImage(QPixmap)), Qt::QueuedConnection);

    connect(&_scene, SIGNAL(rotation90(Rotation90)),
            this, SLOT(_onRotation90(Rotation90)),  Qt::QueuedConnection);

    connect(this, SIGNAL(newOrientation(Rotation90)),
            &_scene, SLOT(newRotation(Rotation90)),  Qt::QueuedConnection);

    connect(&_scene, SIGNAL(resetAspect()), this, SLOT(_resetAspect()),  Qt::QueuedConnection);

    connect(&_scene, SIGNAL(filePosition(QString)), ui->label_imagePosition, SLOT(setText(QString)),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(targetPosition(QString)), ui->label_targetPosition, SLOT(setText(QString)),  Qt::QueuedConnection);

    connect(ui->doubleSpinBox_crop, SIGNAL(valueChanged(double)),
            &_scene, SLOT(newIndividualCrop(double)),  Qt::QueuedConnection);

    connect(ui->splitter, SIGNAL(splitterMoved(int,int)),
            this, SLOT(_updateTargetView(int,int)),  Qt::QueuedConnection);

    connect(ui->radioButton_1_1, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_2_1, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_3_1, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_3_2, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_4_3, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_5_3, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_5_4, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_16_9, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_free, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->radioButton_manual, SIGNAL(toggled(bool)), this, SLOT(_getAspectRatio()));
    connect(ui->lineEdit_aspectRatio, SIGNAL(textChanged(QString)), this, SLOT(_getAspectRatio()));
//    connect(ui->lineEdit_aspectRatio, SIGNAL(textEdited(QString)), this, SLOT(_getAspectRatio()));

    connect(this, SIGNAL(newAspectRatio(double)),
            &_scene, SLOT(newAspectRatio(double)),  Qt::QueuedConnection);

    ui->radioButton_rot0->setChecked(true);

    ui->graphicsView_Source->setBackgroundBrush( QPalette().dark() );//QBrush(IMAGE_BACKGROUND_COLOR));
    ui->graphicsView_Target->setBackgroundBrush(QPalette().dark());//QBrush(IMAGE_BACKGROUND_COLOR));

    //_getAspectRatio();

    connect(&_scene, SIGNAL(fileName(const QString)),
            ui->label_filename, SLOT(setText(QString)), Qt::QueuedConnection);
    connect(&_scene, SIGNAL(fileName(const QString)), SLOT(newFileName(QString)),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(noAspect()), SLOT(noAspect()),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(noRotation()), SLOT(noRotation()),  Qt::QueuedConnection);
    connect(&_scene, SIGNAL(noCrop()), SLOT(noCrop()),  Qt::QueuedConnection);

    if (QIcon::hasThemeIcon("go-first"))
        ui->toolButton_firstImage->setIcon(QIcon::fromTheme("go-first"));
    if (QIcon::hasThemeIcon("go-last"))
        ui->toolButton_lastImage->setIcon(QIcon::fromTheme("go-last"));
    if (QIcon::hasThemeIcon("go-next"))
    {
        ui->toolButton_nextImage->setIcon(QIcon::fromTheme("go-next"));
        ui->toolButton_nextItem->setIcon(QIcon::fromTheme("go-next"));
    }
    if (QIcon::hasThemeIcon("go-previous"))
    {
        ui->toolButton_prevImage->setIcon(QIcon::fromTheme("go-previous"));
        ui->toolButton_prevItem->setIcon(QIcon::fromTheme("go-previous"));
    }

    if (QIcon::hasThemeIcon("zoom-in"))
        ui->toolButton_zoomIn->setIcon(QIcon::fromTheme("zoom-in"));
    if (QIcon::hasThemeIcon("zoom-out"))
        ui->toolButton_zoomOut->setIcon(QIcon::fromTheme("zoom-out"));
    if (QIcon::hasThemeIcon("zoom-original"))
        ui->toolButton_zoomOriginal->setIcon(QIcon::fromTheme("zoom-original"));
    if (QIcon::hasThemeIcon("zoom-fit-best"))
        ui->toolButton_zoomFit->setIcon(QIcon::fromTheme("zoom-fit-best"));

    if (QIcon::hasThemeIcon("view-refresh"))
        ui->toolButton_refresh->setIcon(QIcon::fromTheme("view-refresh"));
    if (QIcon::hasThemeIcon("help"))
        ui->toolButton_Help->setIcon(QIcon::fromTheme("help"));

    connect(ui->toolButton_refresh, SIGNAL(clicked()), &_scene, SLOT(refreshView()),  Qt::QueuedConnection);


    // check for update after two minutes
    QTimer::singleShot( WAIT_FOR_DONATE_HINT_MS, this, SLOT(on_actionSupport_by_donation_triggered()) );
    QTimer::singleShot( WAIT_FOR_CHECK_FOR_UPDATE, this, SLOT(on_actionCheck_for_new_version_triggered()) );

    connect (&_settings, SIGNAL(newValues()), this, SLOT(newValues()),  Qt::QueuedConnection);

    connect (&_scene, SIGNAL(reloadSettings()), this, SLOT(loadSettings()),  Qt::QueuedConnection);
    connect (&_scene, SIGNAL(newCrop(double)), SLOT(onNewCrop(double)),  Qt::QueuedConnection);
    connect (&_scene, SIGNAL(setTargetWaiting(bool)), SLOT(setWaitTarget(bool)),  Qt::QueuedConnection);

    enableButtons();
    newValues();
    _scene.newIndividualCrop(ui->doubleSpinBox_crop->value());

    if (!settings.value("starthelp", false).toBool())
    {
        QTimer::singleShot(2000, this, SLOT(on_actionOnline_Help_triggered()));
    }
    settings.setValue("starthelp", true);


    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this, SLOT(selectAll()));

    _getAspectRatio();
}

void MainWindow::_onLoadFile()
{
    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));

    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.tiff" << "*.tif" << "*.bmp" << "*.png" << "*.gif" << "*.pbm" << "*.pgm" << "*.ppm";

    const QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load Image"),
                                                    settings.value("lastDir").toString(),
                                                    tr("Images (%1)").arg(filters.join(" ")));

    if (filename.length() > 0) {
        QFileInfo info(filename);

        settings.setValue("lastDir", info.absolutePath());

        QFileInfoList otherimages;
        QDir dir(info.absolutePath());
        otherimages = dir.entryInfoList(filters);

        int index = -1;
        for (int i=0; i<(int)otherimages.size(); i++) {
            if (otherimages.at(i).fileName() == info.fileName()) {
                index = i;
            }
        }

        emit newList(otherimages, index);
    }
}

MainWindow::~MainWindow()
{
    _tearDown = true;
    disconnect(this);
    disconnect(&_scene);
    disconnect(&_sceneTarget);
    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));
    settings.setValue("mainwin_geom", saveGeometry());
    settings.setValue("mainwin_splitter", ui->splitter->saveState());
    settings.setValue("cropdistance", ui->doubleSpinBox_cropInitial->value());
    settings.setValue("manualaspect", ui->lineEdit_aspectRatio->text());
    settings.setValue("enforceAspectNextIm", ui->checkBox_enforceAspect->isChecked());

    delete ui;
}

void MainWindow::_onNewTargetImage(const QPixmap& pixmap)
{
    if (_tearDown) return;

    if (_sceneTarget.items().size() > 0)
    {
        for (int i=_sceneTarget.items().size()-1; i>=0; i--) {
            QGraphicsItem* item = _sceneTarget.items()[i];
            _sceneTarget.removeItem(item);
            delete item;
        }
    }

    QGraphicsPixmapItem* pixItem = new QGraphicsPixmapItem(pixmap);
    _sceneTarget.addItem(pixItem);

    _updateTargetView();
}

void MainWindow::_updateTargetView(int, int)
{

    if (_sceneTarget.items().size() > 0) {

        _sceneTarget.setSceneRect(_sceneTarget.items()[0]->boundingRect());
        ui->graphicsView_Target->centerOn(_sceneTarget.items()[0]);
        ui->graphicsView_Target->fitInView(_sceneTarget.items()[0],
                                           Qt::KeepAspectRatio);
        ui->graphicsView_Target->scale(0.95,0.95);
        ui->graphicsView_Target->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->graphicsView_Target->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void MainWindow::_onRotation90(const Rotation90 rotation)
{
    switch (rotation) {
    case TargetRotation0:
        ui->radioButton_rot0->setChecked(true);
        break;
    case TargetRotation90:
        ui->radioButton_rot90->setChecked(true);
        break;
    case TargetRotation180:
        ui->radioButton_rot180->setChecked(true);
        break;
    case TargetRotation270:
        ui->radioButton_rot270->setChecked(true);
        break;
    }
}

void MainWindow::on_radioButton_rot0_toggled(bool isChecked)
{
    if (isChecked) emit newOrientation(TargetRotation0);
}

void MainWindow::on_radioButton_rot90_toggled(bool isChecked)
{
    if (isChecked) emit newOrientation(TargetRotation90);
}

void MainWindow::on_radioButton_rot180_toggled(bool isChecked)
{
    if (isChecked) emit newOrientation(TargetRotation180);
}

void MainWindow::on_radioButton_rot270_toggled(bool isChecked)
{
    if (isChecked) emit newOrientation(TargetRotation270);
}

void MainWindow::on_toolButton_deleteItem_clicked()
{
    _scene.deleteSelection();
}

void MainWindow::on_toolButton_prevItem_clicked()
{
    _scene.nextTarget(-1);
}

void MainWindow::on_toolButton_nextItem_clicked()
{
    _scene.nextTarget(1);
}

void MainWindow::_getAspectRatio()
{
    double aspectRatio = -1;
    if (ui->radioButton_16_9->isChecked()) {
        aspectRatio = 16.0/9.0;
    } else if (ui->radioButton_1_1->isChecked()) {
        aspectRatio = 1.000001;
    } else if (ui->radioButton_2_1->isChecked()) {
        aspectRatio = 2.0;
    } else if (ui->radioButton_3_1->isChecked()) {
        aspectRatio = 3.0/1.0;
    } else if (ui->radioButton_3_2->isChecked()) {
        aspectRatio = 3.0/2.0;
    } else if (ui->radioButton_4_1->isChecked()) {
        aspectRatio = 4.0/1.0;
    } else if (ui->radioButton_4_3->isChecked()) {
        aspectRatio = 4.0/3.0;
    } else if (ui->radioButton_5_3->isChecked()) {
        aspectRatio = 5.0/3.0;
    } else if (ui->radioButton_5_4->isChecked()) {
        aspectRatio = 5.0/4.0;
    } else if (ui->radioButton_manual->isChecked()) {
        QString ratioStr = ui->lineEdit_aspectRatio->text();
        const int pos = ratioStr.indexOf(":");
        if (pos > 0) {
            const double part1 = ratioStr.left(pos).toDouble();
            const double part2 = ratioStr.right(ratioStr.length() -pos -1).toDouble();
            if (part1 > 0 && part2 > 0) {
                aspectRatio = part1 / part2;
            }
        }
        else
        {
            aspectRatio = ratioStr.toFloat();
        }
    }

    emit newAspectRatio(aspectRatio);
}

void MainWindow::toggleAspect(const float aspectNew)
{
    const float minDiff = 0.001;
    float aspect = aspectNew;
    if (aspect < 1.0 && aspect > 0) {
        aspect = 1.0/aspect;
    }
    if (qAbs(aspect - 16.0/9.0) < minDiff){
        ui->radioButton_16_9->setChecked(true);
    } else if (qAbs(aspect - 1.0) < minDiff) {
        ui->radioButton_1_1->setChecked(true);
    } else if (qAbs(aspect - 2.0) < minDiff) {
        ui->radioButton_2_1->setChecked(true);
    } else if (qAbs(aspect - 3.0) < minDiff) {
        ui->radioButton_3_1->setChecked(true);
    } else if (qAbs(aspect - 3.0/2.0) < minDiff) {
        ui->radioButton_3_2->setChecked(true);
    } else if (qAbs(aspect - 4.0) < minDiff) {
        ui->radioButton_4_1->setChecked(true);
    } else if (qAbs(aspect - 4.0/3.0) < minDiff) {
        ui->radioButton_4_3->setChecked(true);
    } else if (qAbs(aspect - 5.0/3.0) < minDiff) {
        ui->radioButton_5_3->setChecked(true);
    } else if (qAbs(aspect - 5.0/4.0) < minDiff) {
        ui->radioButton_5_4->setChecked(true);
    } else if (aspect > 0)
    {
        ui->radioButton_manual->setChecked(true);
        ui->lineEdit_aspectRatio->setText(QString("%1").arg(aspect));
    }
    else
    {
        ui->radioButton_free->setChecked(true);
    }
}

void MainWindow::on_toolButton_zoomIn_clicked()
{
    _scene.zoom(1.25);
}

void MainWindow::on_toolButton_zoomOut_clicked()
{
    _scene.zoom(0.8);
}

void MainWindow::on_toolButton_zoomFit_clicked()
{
    _scene.zoomFit();
}

void MainWindow::on_toolButton_zoomOriginal_clicked()
{
    _scene.zoom1();
}

void MainWindow::on_toolButton_firstImage_clicked()
{
    _scene.loadPosition(0);
    enableButtons();
}

void MainWindow::on_toolButton_lastImage_clicked()
{

    _scene.loadPosition(-2);
    enableButtons();
}

void MainWindow::on_toolButton_nextImage_clicked()
{

    _scene.loadPosition(1, true);
    enableButtons();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    _updateTargetView();
}

void MainWindow::on_toolButton_prevImage_clicked()
{

    _scene.loadPosition(-1, true);
    enableButtons();
}

void MainWindow::on_toolButton_saveTargtes_clicked()
{
    bool save = true;
    bool force = false;
    if (ui->toolButton_saveTargtes->styleSheet().length() > 0)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Already saved"),
                                                                  tr("The images have already been extracted for this scan, "
                                                                     " do you want to save them again?"),
                                                                  QMessageBox::Yes|QMessageBox::Abort);
        save = reply == QMessageBox::Yes;
        force = save;

    }
    if (save)
    {
        _scene.saveCurrent(true, force);
    }
}


void MainWindow::_resetAspect()
{
    ui->radioButton_free->setChecked(true);
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_action_About_triggered()
{
    _about.setModal(true);
    _about.setVersion(version_scannerExtract.getVersionMajor(),
                      version_scannerExtract.getVersionMinor(),
                      version_scannerExtract.getVersionPatch(),
                      version_scannerExtract.getCompilerBits());
    _about.show();
}

void MainWindow::on_actionSupport_by_donation_triggered()
{
    const QString donateText = _about.getDonateText();
    const QString donateTitle = tr("Support Scanned Image Extractor");

    if (sender() == ui->actionSupport_by_donation) {
        QMessageBox::information(this,
                                 donateTitle,
                                 donateText);
    } else {
        // if first time: save date:
        // otherwise: check if one week has passed and asked for donation
        QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));
        const int firstTimeYear = 2000;
        QDateTime firstTime = settings.value("firsttimestarteddonation",
                                             QDateTime(QDate(firstTimeYear,1,1),
                                                       QTime::currentTime())).toDateTime();
        if (firstTime.date().year() == firstTimeYear)
        {
            settings.setValue("firsttimestarteddonation", QDateTime::currentDateTime());
        } else if (QDateTime::currentDateTime().addDays(DAYS_UNTIL_DONATE_HINT) > firstTime ){
            if (!lockDialog()) {
                QTimer::singleShot(10000, this, SLOT(on_actionSupport_by_donation_triggered()));
                return;
            }
            provideInfo(donateTitle, donateText, tr("donatehint"));
            unlockDialog();
            settings.setValue("firsttimestarteddonation", QDateTime::currentDateTime());
        }
    }
}

void MainWindow::on_actionCheck_for_new_version_triggered()
{
    qint32 major, minor, patch;
    QString link, text;
    bool connection;

    QProgressDialog* bar = NULL;
    if (sender() == ui->actionCheck_for_new_version) {
        bar = new QProgressDialog(tr("Retrieving version data"), tr("asdf"), 0, 0, this);
        bar->setCancelButton(0);
        bar->show();
    }

    //QDataStream data(reply);

    const bool result = version_scannerExtract.checkForUpdateVersion(NEW_VERSION_LOCATION_SCANNER_EXTRACT,
                                                            NEW_VERSION_MAGIC_NUMBER,
                                                            major,
                                                            minor,
                                                            patch,
                                                            link,
                                                            text,
                                                            connection);
    if (bar != NULL) {
        bar->close();
        bar->deleteLater();
    }

    // only show once a week
    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));
    settings.setValue("main_geometry", saveGeometry());
    QDate lastHint = settings.value("lastShowUpdate", QDate(1950,1,1)).toDate();
    QDate oneWeekBack = QDate::currentDate().addDays(-7);
    const bool autoCheck = lastHint < oneWeekBack || sender() == ui->actionCheck_for_new_version;

    if (result && autoCheck)
    {
        QDialog *diag = new QDialog(this);
        diag->setModal(true);
        QVBoxLayout* layH = new QVBoxLayout();
        diag->setLayout(layH);
        diag->layout()->addWidget(new QLabel(tr("New version of <i>Scanned Image Extractor</i> available:  <b>%1.%2.%3</b> (current: %4.%5.%6)").arg(major).arg(minor).arg(patch).arg(version_scannerExtract.getVersionMajor()).arg(version_scannerExtract.getVersionMinor()).arg(version_scannerExtract.getVersionPatch())));
        QLabel* dlLabel = new QLabel(QString(tr("Download: <a href=\"%1\">%2</a>")).arg(link).arg(link));
        dlLabel->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);
        dlLabel->setOpenExternalLinks(true);
        diag->layout()->addWidget(dlLabel);
        diag->layout()->addWidget(new QLabel(tr("<b>Changelog:</b>")));
        QTextBrowser* tb = new QTextBrowser();
        tb->setText(text);
        tb->setReadOnly(true);
        diag->layout()->addWidget(tb);
        QPushButton *okay = new QPushButton(tr("close"));
        connect(okay, SIGNAL(clicked()), diag, SLOT(deleteLater()));
        QHBoxLayout *lay = new QHBoxLayout();
        lay->addStretch();
        lay->addWidget(okay);
        lay->addStretch();
        layH->addLayout(lay);
        diag->show();
        settings.setValue("lastShowUpdate", QDate::currentDate());
    } else {
        if (!connection && sender() == ui->actionCheck_for_new_version) {
            QMessageBox::warning(this, tr("no connection"), tr("Was not able to connect to new version data server. Please check manually"), QMessageBox::Ok);
        } else if (sender() == ui->actionCheck_for_new_version) {
            QMessageBox::information(this, tr("no update necessary"), tr("There is no new version of Scanned Image Extractor!"), QMessageBox::Ok);
        }
    }
}

void MainWindow::provideInfo(
                        const QString &headline,
                        const QString &text,
                        const QString& settingsValue,
                        const QString& showAgain,
                        const int width,
                        const int minWidth)
{
    QSettings settings(qApp->translate("tools", IMAGE_ORGANIZATION_ORG), qApp->translate("tools", IMAGE_ORGANIZATION_APP));
    if (!settings.value(settingsValue, true).toBool()) {
        return;
    }

    const int iconsize = 128;

    QDialog* dialog = new QDialog(this, Qt::Dialog);
    dialog->setMinimumWidth(minWidth);
    dialog->setMaximumWidth(width);
    dialog->setModal(true);
    dialog->setWindowTitle(headline);
    QVBoxLayout* layout = new QVBoxLayout;
    QHBoxLayout* messageInfo = new QHBoxLayout;

    QIcon icon;
    if (QIcon::hasThemeIcon("info")) {
        icon = QIcon::fromTheme("info");
    } else {
        icon = QIcon(":/images/info.png");
    }
    QLabel* infoIcon = new QLabel;
    infoIcon->setScaledContents(false);
    pQPixmap pixmap (icon.pixmap(256,
                                 QIcon::Normal,
                                 QIcon::On));
    infoIcon->setPixmap(pixmap->scaled(QSize(iconsize, iconsize),
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));
    infoIcon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    QLabel* info = new QLabel(text);
    info->setOpenExternalLinks(true);
    info->setTextFormat(Qt::RichText);
    info->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    info->setWordWrap(true);
#ifndef _WIN32
    info->setMargin(5);
#endif
    info->setMaximumWidth(width-iconsize);
    messageInfo->addWidget(infoIcon);
    messageInfo->addWidget(info);

    QHBoxLayout* layoutCheckBox = new QHBoxLayout;
    layoutCheckBox->addStretch();
    QCheckBox* showAgainBox = new QCheckBox(showAgain);
    showAgainBox->setChecked(true);
    showAgainBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    layoutCheckBox->addWidget(showAgainBox);

    QHBoxLayout* layoutButton = new QHBoxLayout;
    layoutButton->addStretch();

    QPushButton* okButton = new QPushButton(qApp->translate("tools", "OK"));
    layoutButton->addWidget(okButton);
#ifndef _WIN32
    layoutButton->setMargin(5);
#endif
    qApp->connect(okButton, SIGNAL(clicked()), dialog, SLOT(close()));


    layout->addLayout(messageInfo);
    layout->addLayout(layoutCheckBox);
    layout->addLayout(layoutButton);
    dialog->setLayout(layout);

    dialog->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    dialog->exec();

    settings.setValue(settingsValue, showAgainBox->isChecked());

    delete dialog;
    dialog = 0;
}

bool MainWindow::lockDialog()
{
    if (_dialogLock) {
        return false;
    }
    _dialogLock = true;
    return true;
}

void MainWindow::unlockDialog()
{
    _dialogLock = false;
}

void MainWindow::enableButtons()
{
    bool isFirst, isLast;
    _scene.getIsFirstLast(isFirst, isLast);
    ui->toolButton_nextImage->setEnabled(!isLast);
    ui->toolButton_lastImage->setEnabled(!isLast);
    ui->toolButton_firstImage->setEnabled(!isFirst);
    ui->toolButton_prevImage->setEnabled(!isFirst);
}

void MainWindow::currImageHasChanges(bool hasChanges)
{
    if (hasChanges)
    {
        ui->toolButton_saveTargtes->setStyleSheet("");
        ui->toolButton_saveTargtes->setEnabled(true);
    }
    else
    {
        ui->toolButton_saveTargtes->setStyleSheet("background-color:#AA88EE55;");
        //ui->toolButton_saveTargtes->setEnabled(false);
    }
}

void  MainWindow::newFileName(const QString filename)
{
    setWindowTitle(_title + " - " + filename);
    enableButtons();
}

void MainWindow::on_action_Settings_triggered()
{
    _settings.show();
}

void MainWindow::loadSettings()
{
    _scene.setDestination(_settings.getTargetDir());
    _scene.setPrefix(_settings.getPrefix());

    _scene.setThresh(_settings.getThresh());
    _scene.setMaxAspect(_settings.getMaxAspect());
    _scene.setLevels(_settings.getLevels() );
    _scene.setMaxOverlap(_settings.getMinArea());
    _scene.setMinArea(_settings.getMinArea());
    _scene.setMinAreaWithinImage(_settings.getMinAreaWithinImage());
    _scene.setSplitMaxOffset(_settings.getSplitMaxoffsetFrac());
    _scene.setSplitMinCornerDist(_settings.getSplitMinCornerDist());
    _scene.setSplitMinLengthFrac(_settings.getSplitMinLengthFrac());
    _scene.setMaximumHierarchyLevel(_settings.getMaxHierarchyLevel());
    _scene.setNumPreLoad(_settings.getNumPreLoad());
}

void MainWindow::newValues()
{
    loadSettings();
    _scene.reloadFromNewSettings();
}


void MainWindow::onNewCrop(const double crop)
{
    ui->doubleSpinBox_crop->setValue(crop*100.0);
}


void MainWindow::noRotation()
{
    QListIterator<QObject *> i(ui->groupBox_orientation->children());
    while (i.hasNext())
    {
        QObject* item = i.next();
        QRadioButton* b = dynamic_cast<QRadioButton*>( item );
        if (b != 0 && b->isChecked()) {
            b->setAutoExclusive(false);
            b->setChecked(false);
            b->setAutoExclusive(true);
        }
    }
}

void MainWindow::noAspect()
{
    QListIterator<QObject *> i(ui->groupBox_aspect->children());
    while (i.hasNext())
    {
        QRadioButton* b = dynamic_cast<QRadioButton*>( i.next());
        if (b != 0 && b->isChecked()) {
            b->setAutoExclusive(false);
            b->setChecked(false);
            b->setAutoExclusive(true);
        }
    }
}

void MainWindow::noCrop()
{
    ui->doubleSpinBox_crop->setValue(0);
}

void MainWindow::setWaitTarget(const bool wait)
{
    if (wait) {
        if (_targetWait == 0) {
            _targetWait = new QProgressBar(ui->graphicsView_Target);
            _targetWait->setMaximum(0);
            _targetWait->setMinimum(0);
            _targetWait->show();
        }
    }
    else
    {
        if (_targetWait != 0) {
            _targetWait->setVisible(false);
            delete _targetWait;
            _targetWait = 0;
        }
    }
}



void MainWindow::on_actionAbout_liblbfgs_triggered()
{
    QMessageBox::information(this,
                             tr("liblbfgs"),
                             tr("<a href=\"http://www.chokkan.org/software/liblbfgs/\">liblbfgs</a> is an optimization/energy minimization library which implements the"
                                " Limited-memory Broyden-Fletcher-Goldfarb-Shanno algorithm (L-BFGS)."
                                "Copyright (c) 2002-2014 by Naoaki Okazaki - <a href=\"http://opensource.org/licenses/mit-license.php\">MIT license</a>"),
                             QMessageBox::Ok);


}

void MainWindow::on_doubleSpinBox_cropInitial_valueChanged(const double val)
{
    _scene.initialCropChanged(val/100.0);
}

void MainWindow::numSelectionChanged(const int num)
{
    ui->doubleSpinBox_crop->setEnabled(num > 0);
    ui->doubleSpinBox_cropInitial->setEnabled(num == 0);
}


void MainWindow::doneCopy(const QString& filename,
                          const QString& targetLocation)
{
    statusBar()->showMessage(QString(tr("finished copying '%1' to '%2'")).arg(filename).arg(targetLocation),
                             5000);
}

void MainWindow::on_actionOnline_Help_triggered()
{
    _helpDialog.show();
    /*
    QMessageBox::information(this,
                             tr("Help"),getHelpText(),
                             QMessageBox::Ok);*/
}

QString MainWindow::getHelpText()
{

    return tr("Find the online help at <a href=\"http://dominik-ruess.de/scannerExtract\">dominik-ruess.de/scannerExtract</a>"
              "<br><br>"
              "<b>Workflow:</b><ol>"
              "<li>load a scanned image</li>"
              "<li>the system will suggest photographs, you can now:<ol>"
              "<li>accept the suggestion(s)</li>"
              "<li>manipulate suggestions: <ul><li>drag corner or edge of rectangles</li>"
              "<li>press CTRL for symmetric change</li>"
              "<li>keep SHIFT pressed before dragging corner, this rotates the rectangle</li></ul></li>"
              "<li>add new rectangle: deselect all (click somewhere empty). Click on a photograph corner, keep mouse clicked and drag line to a second corner. Then move/resize the new rectangle and click to release.</li>"
              "</ol></li>"
              "<li>the rectangles will be saved automatically when you go to the next scanned image</li></ol>"
              "<b>Keyboard shortcuts:</b><br><table>"
              "<tr><td>Keys 0-9</td><td> select aspect ratios</td></tr>"
              "<tr><td>Keys 'a', 's', 'd' and 'f'</td><td> change orientation of current target</td></tr>"
              "<tr><td>Keys CTRL+V and CTRL+B</td><td> navigate to prev. and next input image</td></tr>"
              "<tr><td>Keys N, M and delete</td><td> navigate prev. and next target or delete target</td></tr></table>");
}

void MainWindow::showStartupHelp()
{
    provideInfo(tr("Startup Hint"), getHelpText(), tr("startupHelp"));
}

void MainWindow::on_toolButton_Help_clicked()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::on_actionAbout_Open_CV_triggered()
{
    QMessageBox::information(this,
                             tr("About OpenCV"),
                             tr("<a href=\"http://opencv.org\">OpenCV</a> a powerful and free (BSD-License) computer vision library."
                                " Scanned Image Extractor can use version 2 or 3 of OpenCV."
                                " Copyright (c) 2015 by <a href=\"http://itseez.com/\">itseez</a>"),
                             QMessageBox::Ok);
}
