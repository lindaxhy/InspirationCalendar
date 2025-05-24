#include "AIService.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QJsonObject>

AIService::AIService(QObject *parent)
    : QObject(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_apiEndpoint("https://api.deepseek.com/chat/completions"), // DeepSeek API端点
    m_currentReply(nullptr),
    m_progressDialog(nullptr)
{
    //connect(m_networkManager, &QNetworkAccessManager::finished,
    //this, &AIService::handleApiResponse);
}

void AIService::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void AIService::setApiEndpoint(const QString &endpoint)
{
    m_apiEndpoint = endpoint;
}

void AIService::generateHistoricalAnalysis(const QString &eventTitle,
                                           const QString &question,
                                           const QString &choice,
                                           QWidget *parentWidget)
{
    if (m_apiKey.isEmpty()) {
        showError("API密钥未设置", parentWidget);
        return;
    }

    // 准备提示词 - 针对历史分析场景优化
    QString prompt = QString(
                         "作为历史模拟专家，请基于用户选择生成分析：\n"
                         "【历史事件】%1\n"
                         "【用户选择】%2\n\n"
                         "生成要求：\n"
                         "- 事实部分(60%篇幅)：包含3个关键数据点\n"
                         "- 对比分析(30%篇幅)：分'短期(5年内)'和'长期(20年后)'两个阶段\n"
                         "- 互动设计(10%篇幅)：\n"
                         "  a) 1个追问选择（如：'想深入了解[技术/政治/经济]哪个方面的影响？'）\n"
                         "  b) 1个预测验证（如：'您认为俄罗斯航天市场份额会超过__%？'）\n\n"
                         "JSON格式要求：\n"
                         "{\n"
                         "  \"facts\": [\"...\", \"...\"],  // 改为数组形式\n"
                         "  \"analysis\": {\n"
                         "    \"short_term\": \"...\",\n"
                         "    \"long_term\": \"...\"\n"
                         "  },\n"
                         "  \"interaction\": {\n"
                         "    \"follow_up\": \"...\",\n"
                         "    \"prediction_test\": \"...\"\n"
                         "  }\n"
                         "}"
                         ).arg(eventTitle).arg(choice);

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    if (m_progressDialog) {
        m_progressDialog->deleteLater();
    }
    // 显示进度对话框
    m_progressDialog = new QProgressDialog(
        "正在与DeepSeek时空模拟器交互...",
        "取消",
        0,
        0,
        parentWidget);
    m_progressDialog->setWindowModality(Qt::WindowModal);

    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
        emit errorOccurred("用户取消了操作");
    });

    QNetworkRequest request{QUrl(m_apiEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    QJsonObject requestData = constructApiRequest(prompt);
    m_currentReply = m_networkManager->post(request, QJsonDocument(requestData).toJson());

    // 连接信号（Qt5 & Qt6 兼容写法）
    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        handleApiResponse(m_currentReply);  // 显式传递当前reply
    });
}

QJsonObject AIService::constructApiRequest(const QString &prompt)
{
    QJsonObject data;
    data["model"] = "deepseek-chat"; // DeepSeek的模型名称
    data["temperature"] = 1.0;
    data["max_tokens"] = 1000;      // DeepSeek支持更长的响应
    data["stream"] = false;

    QJsonArray messages;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] =
        "你是一个历史模拟专家，专门分析历史事件的平行时空可能性。"
        "必须严格保持75%历史事实和25%合理推演的比例。"
        "必须返回有效的JSON格式。";
    messages.append(systemMessage);

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);

    data["messages"] = messages;
    return data;
}

void AIService::sendFollowUpQustion(const QString& question, const ConversationContext& context) {
    // 1. 验证上下文有效性
    if (context.lastResponse.isEmpty()) {
        emit conversationError("无效的对话上下文");
        return;
    }

    // 2. 构建智能提示词
    QString prompt = QString(
                         "作为历史模拟对话AI，请基于以下上下文回答追问：\n"
                         "【历史事件】%1\n"
                         "【初始选择】%2\n"
                         "【上次分析】%3\n\n"
                         "【用户追问】%4\n\n"
                         "回答要求：\n"
                         "- 保持原有JSON格式\n"
                         "- 新增'depth_analysis'字段包含技术/政治/经济三个维度分析\n"
                         "- 在'interaction'中添加'next_options'提供2-3个后续探索方向\n\n"
                         "响应格式示例：\n"
                         "{\n"
                         "  \"rephrased_question\": \"重新表述的问题\",\n"
                         "  \"depth_analysis\": {\n"
                         "    \"technology\": \"...\",\n"
                         "    \"politics\": \"...\",\n"
                         "    \"economy\": \"...\"\n"
                         "  },\n"
                         "  \"next_options\": [\"选项1\", \"选项2\"]\n"
                         "}"
                         ).arg(context.eventTitle,
                              context.initialChoice,
                              QJsonDocument(context.lastResponse).toJson(),
                              question);

    // 3. 准备API请求
    QNetworkRequest request{QUrl(m_apiEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    QJsonObject requestData;
    requestData["model"] = "deepseek-chat";
    requestData["temperature"] = 0.7;

    // 4. 构建对话历史（保留最近3轮）
    QJsonArray messages;
    messages.append(QJsonObject{
        {"role", "system"},
        {"content", "你正在参与一个历史模拟的深度对话，需保持专业性和连贯性"}
    });

    if (!m_contextHistory.isEmpty()) {
        auto startIt = m_contextHistory.end() - qMin(3, m_contextHistory.size());
        for (auto it = startIt; it != m_contextHistory.end(); ++it) {
            QJsonObject assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = QString::fromUtf8(QJsonDocument(it->lastResponse).toJson());
            messages.append(assistantMsg);
        }
    }

    messages.append(QJsonObject{
        {"role", "user"},
        {"content", prompt}
    });

    requestData["messages"] = messages;

    // 5. 发送请求
    QNetworkReply* reply = m_networkManager->post(
        request,
        QJsonDocument(requestData).toJson()
        );

    // 6. 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit conversationError("API请求失败: " + reply->errorString());
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit conversationError("响应解析失败");
            return;
        }

        QJsonObject response = doc.object();

        // 7. 更新上下文
        ConversationContext newContext = context;
        newContext.lastResponse = response;
        newContext.timestamp = QDateTime::currentDateTime();

        m_contextHistory.append(newContext);
        if (m_contextHistory.size() > 3) {
            m_contextHistory.removeFirst();
        }

        // 8. 发射信号
        emit followUpReady(response);
    });
}

void AIService::handleApiResponse(QNetworkReply *reply)
{
    if (!reply || reply != m_currentReply) {
        if (reply) reply->deleteLater();
        qDebug() << "警告：收到无效或过期的 reply 对象";
        return;
    }
    m_currentReply = nullptr;
    // 先处理数据再关闭对话框（重要修改点）
    QByteArray data = reply->readAll();
    reply->deleteLater();

    qDebug() << "原始响应数据:" << data;

    // 检查是否是用户取消
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        qDebug() << "操作已被用户取消";
        if (m_progressDialog) {
            m_progressDialog->deleteLater();
            m_progressDialog = nullptr;
        }
        return;
    }

    // 处理成功响应
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred("JSON解析错误: " + parseError.errorString());
        return;
    }

    // 确保对话框关闭
    if (m_progressDialog) {
        // 修改点：使用disconnect防止误触发
        m_progressDialog->disconnect();
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }

    // 提取AI生成的内容（关键修改）
    if (jsonDoc.isObject()) {
        QJsonObject rootObj = jsonDoc.object();

        // 处理DeepSeek API的标准响应结构
        if (rootObj.contains("choices")) {
            QJsonArray choices = rootObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QString content = choices[0].toObject()["message"].toObject()["content"].toString();
                qDebug() << "AI生成内容:" << content;

                QJsonDocument contentDoc = QJsonDocument::fromJson(content.toUtf8());
                if (contentDoc.isObject()) {
                    emit analysisReady(contentDoc.object());
                    return;
                }
            }
        }

        // 如果已经是直接返回的有效JSON（调试时的情况）
        emit analysisReady(rootObj);
    } else {
        emit errorOccurred("API返回了无效的响应格式");
    }
}

QJsonObject AIService::parseAiResponse(const QByteArray &response)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
    if (jsonResponse.isNull()) {
        qDebug() << "无效的JSON响应";
        return QJsonObject();
    }

    QJsonObject rootObj = jsonResponse.object();
    if (!rootObj.contains("choices")) {
        qDebug() << "响应缺少choices字段";
        return QJsonObject();
    }

    QJsonArray choices = rootObj["choices"].toArray();
    if (choices.isEmpty()) {
        qDebug() << "choices数组为空";
        return QJsonObject();
    }

    QString aiContent = choices[0].toObject()["message"].toObject()["content"].toString();
    qDebug() << "AI原始内容:" << aiContent;

    QJsonDocument aiDoc = QJsonDocument::fromJson(aiContent.toUtf8());
    if (aiDoc.isNull()) {
        qDebug() << "AI内容不是有效的JSON";
        return QJsonObject();
    }

    return aiDoc.object();
}

void AIService::showError(const QString &message, QWidget *parent)
{
    if (parent) {
        QMessageBox::warning(parent, "DeepSeek API错误", message);
    }
    emit errorOccurred(message);
}
