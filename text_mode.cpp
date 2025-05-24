#include "text_mode.h"
#include "ui_text_mode.h"
#include "mainwindow.h"
#include "color_mode.h"
#include <QFile>
QStringList chineselist={"黑体","楷体","宋体"};
QStringList englishlist={"TimesNewRoman","Calibri","Arial"};
int currentLanguage = 0;
QString currentFont;

text_mode::text_mode(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::text_mode)
{
    ui->setupUi(this);
    this->setWindowTitle("个性化设置");

    //添加语言
    ui->cbo_language->addItem("中文");
    ui->cbo_language->addItem("Enlish");

    connect(ui->cbo_language,SIGNAL(currentIndexChanged(int)),this,SLOT(on_cbo_language_currentIndexChanged(int)));
    // 绑定字体选择变化槽
    connect(ui->cbo_text, SIGNAL(currentTextChanged(const QString&)), this, SLOT(on_cbo_text_currentTextChanged(const QString &)));
    // 初始化字体下拉框，默认语言0
    on_cbo_language_currentIndexChanged(0);
    if (!chineselist.isEmpty()) {
        currentFont = chineselist.at(0);
    }
}

text_mode::~text_mode()
{
    delete ui;
}

void text_mode::on_cbo_text_currentTextChanged(const QString &fontName) {
    if (currentLanguage == 0) { // 中文
        if (fontName == "黑体") {
            currentFont = "SimHei";  // 黑体对应 SimHei
        } else if (fontName == "楷体") {
            currentFont = "KaiTi";   // 楷体对应 KaiTi
        } else if (fontName == "宋体") {
            currentFont = "SimSun";  // 宋体对应 SimSun
        } else {
            currentFont = fontName;  // 其他情况直接用名字
        }
    } else { // 英文
        currentFont = fontName;
    }
    qDebug() << currentFont << "currentFont";
}

void text_mode::on_btn_confirm_clicked()
{
    MainWindow *w=new MainWindow();
    w->setLanguage(currentLanguage);
    w->setFontName(currentFont);
    w->setColorMode(colorMode);
    w->show();
    this->hide();
}


void text_mode::on_cbo_language_currentIndexChanged(int index)
{
    currentLanguage = index;
    ui->cbo_text->clear();
    switch(index){
    case 0:
        ui->cbo_text->addItems(chineselist);
        break;
    case 1:
        ui->cbo_text->addItems(englishlist);
        break;
    }
}

void text_mode::setColorMode(const QString &mode) {
    colorMode = mode;
}
void text_mode::on_btn_return_clicked()
{
    color_mode *w=new color_mode();
    w->show();
    this->hide();
}


