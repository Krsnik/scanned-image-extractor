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

#include <stdint.h>
#include "about.h"
#include "ui_about.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QFile>

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);

    const int size = 128;
    ui->label_image->setMargin(5);
    ui->label_image->setMinimumSize(size+10, size+10);
    ui->label_image->setMaximumSize(size+10, size+10);
    ui->label_image->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    //ui->label_image->setMaximumSize(size, size);
    ui->label_image->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


    QSvgRenderer renderer(QString(":images/logo.svg"));
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(0x00000000);  // partly transparent red-ish background
    QPainter painter(&image);
    renderer.render(&painter);

    QString text;
    QFile tmp(tr(":WARRANTY_EN"));
    if (tmp.open(QIODevice::ReadOnly))
    {
        text = tmp.readAll();
    }
    ui->label_license->setText(ui->label_license->text() + text);
    ui->label_license->setWordWrap(true);

    ui->label_image->setPixmap(QPixmap::fromImage(image));//.scaled(size, size, Qt::KeepAspectRatio));
    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

DialogAbout::~DialogAbout()
{
    delete ui;
}

void DialogAbout::on_pushButton_done_clicked()
{
    this->close();
}

void DialogAbout::setVersion(const int major, const int minor, const int patch, const int bits)
{
    ui->label_version->setText(tr("version %1.%2.%3 - %4bit version").arg(major).arg(minor).arg(patch).arg(bits));
}

QString DialogAbout::getDonateText() const
{
    return ui->label_donation->text();
}
