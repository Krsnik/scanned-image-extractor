#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);

    ui->label_overview->setPixmap(QPixmap::fromImage(QImage(":/images/overview.png")));
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
