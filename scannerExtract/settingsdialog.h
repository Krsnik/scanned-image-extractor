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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "settings.h"
#include "ui_settingsdialog.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

signals:
    void newValues();

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    double getThresh() const { return ui->doubleSpinBox_thres->value(); }
    double getMaxAspect() const { return ui->doubleSpinBox_maxAspect->value(); }

    QString getTargetDir() const { return ui->lineEdit_location->text(); }
    QString getPrefix() const { return ui->lineEdit_prefix->text(); }

    int getLevels() const { return ui->spinBox_levels->value(); }
    double getMaxOverlap() const { return (double)ui->spinBox_maxOverlap->value() / 100.0; }
    double getMinArea() const { return (double)ui->spinBox_minArea->value() / 100.0 ; }
    double getMinAreaWithinImage() const { return (double)ui->spinBox_minAreaWithinImage->value() / 100.0 ; }
    double getSplitMaxoffsetFrac() const { return (double)ui->spinBox_splitMaxOffsetFrac->value() / 100.0 ; }
    double getSplitMinCornerDist() const { return (double)ui->spinBox_splitMinCornerDist->value() / 100.0 ; }
    double getSplitMinLengthFrac() const { return (double)ui->spinBox_splitMinLengthFrac->value() / 100.0 ; }
    double getMaxHierarchyLevel() const { return ui->spinBox_maxHierarchyLevel->value(); }
    int getNumPreLoad() const { return ui->spinBox_preload->value(); }

private slots:
    void setTargetDir();

    void save();
    void load();

    void accepted();

    void rejected();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
