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

#include "settingsdialog.h"

#include <QSettings>
#include <QDir>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->tabWidget->removeTab(1);
    load();

    connect(ui->toolButton_folder, SIGNAL(clicked()), this, SLOT(setTargetDir()));

    connect(ui->buttonBox, SIGNAL(accepted()), SLOT(accepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), SLOT(rejected()));
}

void SettingsDialog::load()
{
    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));


    QString home = QDir::homePath() + "/" + tr("Pictures") + "/" + tr("ScannedImageExtractor") + "/";
    ui->lineEdit_location->setText(settings.value("targetdir", home).toString());
    ui->lineEdit_prefix->setText(settings.value("prefix", "fromScanner_").toString());

    ui->doubleSpinBox_thres->setValue(settings.value("thresh", 5.0).toDouble());
    ui->doubleSpinBox_maxAspect->setValue(settings.value("max aspect", 2.1).toDouble());

    ui->spinBox_levels->setValue(settings.value("levels", 3).toInt());
    ui->spinBox_maxOverlap->setValue(settings.value("maxOverlap", 30).toInt());
    ui->spinBox_minArea->setValue(settings.value("minArea", 10.0).toInt());
    ui->spinBox_minAreaWithinImage->setValue(settings.value("minAreaWithinImage", 90).toInt());
    ui->spinBox_splitMaxOffsetFrac->setValue(settings.value("splitMaxOffsetFromDiag", 20).toInt());
    ui->spinBox_splitMinCornerDist->setValue(settings.value("splitMinCornerDist", 30).toInt());
    ui->spinBox_splitMinLengthFrac->setValue(settings.value("splitMinLengthFrac", 35).toInt());
    ui->spinBox_maxHierarchyLevel->setValue(settings.value("maxHierarchyLevel", 15).toInt());
    ui->spinBox_preload->setValue(settings.value("preload", 10).toInt());
}

void SettingsDialog::save()
{

    QSettings settings(tr(IMAGE_ORGANIZATION_ORG), tr(IMAGE_ORGANIZATION_APP));

    QString dir = ui->lineEdit_location->text();
    dir.replace("\\", "/");
    if (dir.right(1) != "/")
    {
        dir = dir + "/";
    }
    ui->lineEdit_location->setText(dir);
    settings.setValue("targetdir", dir);
    settings.setValue("prefix", ui->lineEdit_prefix->text());

    settings.setValue("thresh", ui->doubleSpinBox_thres->value());
    settings.setValue("max aspect", ui->doubleSpinBox_maxAspect->value());

    settings.setValue("levels", ui->spinBox_levels->value());
    settings.setValue("maxOverlap", ui->spinBox_maxOverlap->value());
    settings.setValue("minArea", ui->spinBox_minArea->value());
    settings.setValue("minAreaWithinImage", ui->spinBox_minAreaWithinImage->value());
    settings.setValue("splitMaxOffsetFromDiag", ui->spinBox_splitMaxOffsetFrac->value());
    settings.setValue("splitMinCornerDist", ui->spinBox_splitMinCornerDist->value());
    settings.setValue("splitMinLengthFrac", ui->spinBox_splitMinLengthFrac->value());
    settings.setValue("maxHierarchyLevel", ui->spinBox_maxHierarchyLevel->value());

    settings.setValue("preload", ui->spinBox_preload->value());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accepted()
{
    // save and close
    save();
    emit newValues();
    close();
}

void SettingsDialog::rejected()
{
    // reset and close
    load();
    close();
}


void SettingsDialog::setTargetDir()
{
    QString location = ui->lineEdit_location->text();
    location = QFileDialog::getExistingDirectory(this,
                                                 tr("Select Output directory"),
                                                 location);

    if (location.length() == 0) {
        return;
    }

    ui->lineEdit_location->setText(QDir(location).canonicalPath() );
}
