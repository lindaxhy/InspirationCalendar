#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "AIService.h"
#include "weathermanager.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include <QDate>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class InspirationCalendarWidget;  // 前向声明（拼写修正）

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void setColorMode(const QString &mode);
    void loadStyleSheet(const QString &fileName);
    void setLanguage(int languageIndex); // 0-中文,1-英文
    void setFontName(const QString &fontName);
    ~MainWindow();

private slots:
    //void onDateClicked(const QDate &date);
    void onInspirationClicked();
    void onHistoryEventClicked();
    void onExportStarMapClicked();
    void weatherUpdatedUI(); // 更新天气UI的槽函数

private:
    Ui::MainWindow *ui;
    int currentLanguage = 0;
    AIService *m_aiService;
    QString currentFontName;
    QMap<QDate, QString> notes;                // 保存每一天的灵感手账
    QMap<QDate, int> inspirationScoreMap;      // 灵感值记录
    InspirationCalendarWidget *calendarWidget; // 自定义日历控件
    void loadInspirationData();
    void saveInspirationData();
    void handleAiAnalysis(const QJsonObject &result);
    void handleAiError(const QString &errorMessage);
    QString dataFilePath() const;
    WeatherManager *weathermanager; // 天气数据对象
    QList<QString> m_followUpOptions;
};

#endif // MAINWINDOW_H
