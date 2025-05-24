#include "color_mode.h"
#include "ui_color_mode.h"
#include "text_mode.h"
#include "everyday_sentence.h"
#include <QMessageBox>
color_mode::color_mode(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::color_mode)
{
    ui->setupUi(this);
    this->setWindowTitle("个性化设置");
}

color_mode::~color_mode()
{
    delete ui;
}

void color_mode::on_btn_confirm_clicked()
{
    QString colorMode;
    if (ui->cbBlack->isChecked())
        colorMode = "black";
    else if (ui->cbWhite->isChecked())
        colorMode = "white";
    if (!colorMode.isEmpty()) {
        text_mode *w = new text_mode();
        w->setColorMode(colorMode);
        w->show();
        this->hide();
    }
    else {
        QMessageBox::about(this, "提示", "请选择颜色模式");
    }
}


void color_mode::on_btn_return_clicked()
{
    everyday_sentence *w=new everyday_sentence();
    w->show();
    this->hide();
}


void color_mode::on_cbBlack_stateChanged()
{
    ui->cbWhite->setCheckState(Qt::Unchecked);
}


void color_mode::on_cbWhite_stateChanged()
{
    ui->cbBlack->setCheckState(Qt::Unchecked);
}

