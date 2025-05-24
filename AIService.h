#ifndef AISERVICE_H
#define AISERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QProgressDialog>

class AIService : public QObject
{
    Q_OBJECT

public:
    explicit AIService(QObject *parent = nullptr);

    // 设置API密钥
    void setApiKey(const QString &apiKey);

    // 主调用方法
    void generateHistoricalAnalysis(const QString &eventTitle,
                                    const QString &question,
                                    const QString &choice,
                                    QWidget *parentWidget = nullptr);
    void setApiEndpoint(const QString &endpoint);
    struct ConversationContext {
        QString eventTitle;
        QString initialChoice;
        QJsonObject lastResponse;
        QDateTime timestamp;
    };
    void sendFollowUpQustion(const QString& question, const ConversationContext& context);

signals:
    // 信号：当AI响应准备好时发出
    void analysisReady(const QJsonObject &result);
    void errorOccurred(const QString &errorMessage);
    void followUpReady(const QJsonObject& response);
    void conversationError(const QString &message);

private slots:
    void handleApiResponse(QNetworkReply* reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QNetworkReply *m_currentReply;
    QString m_apiEndpoint;
    QProgressDialog *m_progressDialog;
    ConversationContext m_currentContext;
    QList<ConversationContext> m_contextHistory;
    // 私有方法
    QJsonObject constructApiRequest(const QString &prompt);
    QJsonObject parseAiResponse(const QByteArray &response);
    void showError(const QString &message, QWidget *parent);
};

#endif // AISERVICE_H
