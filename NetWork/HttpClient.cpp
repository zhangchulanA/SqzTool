#include "HttpClient.h"
#include <QDebug>
#include <QPointer>

// ================= HttpRequest 实现 =================

HttpRequest::HttpRequest(QNetworkAccessManager *manager,
                         const QString &baseUrl,
                         const QString &path,
                         Method method,
                         QObject *parent)
    : QObject(parent),
      m_manager(manager),
      m_baseUrl(baseUrl),
      m_path(path),
      m_method(method)
{
    // 超时定时器在 execute 中创建并绑定到 reply，此处无需初始化
}

HttpRequest::~HttpRequest()
{
    // 如果仍有活跃 reply，则取消
    if (m_currentReply) {
        disconnect(m_currentReply, nullptr, this, nullptr);
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
    }
}

// ---------- 链式配置 ----------

HttpRequest& HttpRequest::header(const QString &key, const QString &value)
{
    m_headers[key] = value;
    return *this;
}

HttpRequest& HttpRequest::param(const QString &key, const QVariant &value)
{
    m_params[key] = value;
    return *this;
}

HttpRequest& HttpRequest::jsonBody(const QJsonObject &json)
{
    m_body = QJsonDocument(json).toJson(QJsonDocument::Compact);
    m_contentType = "application/json";
    return *this;
}

HttpRequest& HttpRequest::formBody(const QVariantMap &form)
{
    QUrlQuery query;
    for (auto it = form.begin(); it != form.end(); ++it)
        query.addQueryItem(it.key(), it.value().toString());
    m_body = query.toString(QUrl::FullyEncoded).toUtf8();
    m_contentType = "application/x-www-form-urlencoded";
    return *this;
}

HttpRequest& HttpRequest::rawBody(const QByteArray &data, const QString &contentType)
{
    m_body = data;
    m_contentType = contentType;
    return *this;
}

HttpRequest& HttpRequest::timeout(int ms)
{
    m_timeoutMs = ms;
    return *this;
}

HttpRequest& HttpRequest::retry(int count)
{
    m_retryCount = count;
    return *this;
}

HttpRequest& HttpRequest::polling(int intervalMs, int maxAttempts, PollContinueCallback shouldContinue)
{
    m_pollIntervalMs = intervalMs;
    m_maxPollAttempts = maxAttempts;
    m_pollContinue = shouldContinue;
    return *this;
}

HttpRequest& HttpRequest::success(SuccessCallback cb)
{
    m_successCallback = cb;
    return *this;
}

HttpRequest& HttpRequest::error(ErrorCallback cb)
{
    m_errorCallback = cb;
    return *this;
}

HttpRequest& HttpRequest::progress(ProgressCallback cb)
{
    m_progressCallback = cb;
    return *this;
}

// ---------- 取消 ----------

void HttpRequest::cancel()
{
    if (m_cancelled)
        return;
    m_cancelled = true;
    if (m_currentReply) {
        disconnect(m_currentReply, nullptr, this, nullptr);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    // 取消后触发错误回调（如果已设置）并删除对象
    HttpResponse resp;
    resp.success = false;
    resp.errorString = tr("Request cancelled");
    resp.networkError = QNetworkReply::OperationCanceledError;
    if (m_errorCallback)
        m_errorCallback(resp);
    cleanup();
}

// ---------- 发送 ----------

void HttpRequest::send()
{
    if (m_sent) {
        qWarning() << "HttpRequest::send() called more than once, ignoring.";
        return;
    }
    m_sent = true;
    m_currentRetry = 0;
    m_currentPollAttempt = 0;
    execute();
}

// ---------- 私有方法 ----------

QUrl HttpRequest::buildUrl() const
{
    QString fullUrl;
    if (m_path.startsWith("http://") || m_path.startsWith("https://")) {
        fullUrl = m_path;
    } else {
        fullUrl = m_baseUrl;
        if (!fullUrl.isEmpty()) {
            if (fullUrl.endsWith('/') && m_path.startsWith('/'))
                fullUrl.chop(1);
            else if (!fullUrl.endsWith('/') && !m_path.startsWith('/'))
                fullUrl += '/';
        }
        fullUrl += m_path;
    }

    QUrl url(fullUrl);
    // 合并原有查询参数和 m_params
    QUrlQuery query(url);
    // 保留原有参数
    for (auto it = m_params.begin(); it != m_params.end(); ++it)
        query.addQueryItem(it.key(), it.value().toString());
    url.setQuery(query);
    return url;
}

QNetworkRequest HttpRequest::buildRequest() const
{
    QNetworkRequest request(buildUrl());
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    if (!m_contentType.isEmpty())
        request.setHeader(QNetworkRequest::ContentTypeHeader, m_contentType);
    return request;
}

void HttpRequest::execute()
{
    if (m_cancelled) {
        cleanup();
        return;
    }

    QNetworkRequest request = buildRequest();
    qDebug() << "[HTTP]" << static_cast<int>(m_method) << request.url().toString();

    QNetworkReply *reply = nullptr;
    switch (m_method) {
    case Method::Get:    reply = m_manager->get(request); break;
    case Method::Post:   reply = m_manager->post(request, m_body); break;
    case Method::Put:    reply = m_manager->put(request, m_body); break;
    case Method::Delete: reply = m_manager->deleteResource(request); break;
    }

    m_currentReply = reply;

    // 连接进度信号
    if (m_progressCallback) {
        connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 sent, qint64 total) {
            if (m_progressCallback) m_progressCallback(sent, total);
        });
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
            if (m_progressCallback) m_progressCallback(received, total);
        });
    }

    // 超时定时器（绑定到 reply，自动清理）
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() { reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        timer->stop();
        // 可能 reply 已被取消或替换，检查是否当前 reply
        if (m_currentReply == reply) {
            handleReply(reply);
            // 注意：handleReply 中可能删除 this，所以之后不再使用成员
        }
        reply->deleteLater();
        m_currentReply = nullptr;
    });
    timer->start(m_timeoutMs);
    m_timeoutTimer = timer; // 保存以备析构时清理
}

void HttpRequest::handleReply(QNetworkReply *reply)
{
    if (m_cancelled) {
        cleanup();
        return;
    }

    HttpResponse response;
    response.rawData = reply->readAll();
    response.networkError = reply->error();

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid())
        response.statusCode = statusCode.toInt();

    bool networkOk = (reply->error() == QNetworkReply::NoError);
    bool httpOk = (response.statusCode >= 200 && response.statusCode < 300);
    response.success = networkOk && httpOk;

    // 解析 JSON
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(response.rawData, &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        if (doc.isObject())
            response.jsonObject = doc.object();
        else if (doc.isArray())
            response.jsonArray = doc.array();
    }

    if (!response.success)
        response.errorString = reply->errorString();

    // ---------- 最终成功判定 ----------
    if (response.success) {
        // 成功：无论是否有轮询条件，成功时若轮询条件存在且要求继续，则继续轮询；否则结束
        bool shouldContinuePolling = false;
        if (m_pollContinue) {
            shouldContinuePolling = m_pollContinue(response);
        } else if (m_pollIntervalMs > 0) {
            // 兼容旧行为：仅当响应失败时才轮询，但这里成功，所以不轮询
            shouldContinuePolling = false;
        }

        if (shouldContinuePolling) {
            // 继续轮询（即使成功，但条件决定继续）
            m_currentPollAttempt++;
            if (m_maxPollAttempts == -1 || m_currentPollAttempt < m_maxPollAttempts) {
                qDebug() << "[HTTP POLL] success but continue, attempt" << m_currentPollAttempt
                         << "next in" << m_pollIntervalMs << "ms";
                QTimer::singleShot(m_pollIntervalMs, this, [this]() { execute(); });
                return; // 保留对象
            }
        }

        // 最终成功
        if (m_successCallback)
            m_successCallback(response);
        cleanup();
        return;
    }

    // ---------- 失败处理 ----------

    // 1) 普通重试（不计入轮询次数）
    if (m_retryCount > 0 && m_currentRetry < m_retryCount) {
        m_currentRetry++;
        qDebug() << "[HTTP RETRY] attempt" << m_currentRetry;
        execute();
        return;
    }

    // 2) 轮询（失败情况下，或自定义条件）
    bool shouldPoll = false;
    if (m_pollContinue) {
        // 调用自定义条件（即使失败也调用）
        shouldPoll = m_pollContinue(response);
    } else if (m_pollIntervalMs > 0) {
        // 默认失败时轮询
        shouldPoll = true;
    }

    if (shouldPoll) {
        m_currentPollAttempt++;
        if (m_maxPollAttempts == -1 || m_currentPollAttempt < m_maxPollAttempts) {
            qDebug() << "[HTTP POLL] failure, attempt" << m_currentPollAttempt
                     << "retry in" << m_pollIntervalMs << "ms";
            QTimer::singleShot(m_pollIntervalMs, this, [this]() { execute(); });
            return; // 保留对象
        }
    }

    // ---------- 最终失败 ----------
    if (m_errorCallback)
        m_errorCallback(response);
    cleanup();
}

void HttpRequest::cleanup()
{
    // 取消任何定时器
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }
    // 删除自身（因为是 QObject 子对象，父对象会管理，但为了立即释放，调用 deleteLater）
    deleteLater();
}

// ================= HttpClient 实现 =================

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
}

void HttpClient::setBaseUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl;
}

void HttpClient::setDefaultHeader(const QString &key, const QString &value)
{
    m_defaultHeaders[key] = value;
}

void HttpClient::setBearerToken(const QString &token)
{
    m_defaultHeaders["Authorization"] = "Bearer " + token;
}

HttpRequest& HttpClient::get(const QString &path)
{
    return createRequest(path, HttpRequest::Method::Get);
}

HttpRequest& HttpClient::post(const QString &path)
{
    return createRequest(path, HttpRequest::Method::Post);
}

HttpRequest& HttpClient::put(const QString &path)
{
    return createRequest(path, HttpRequest::Method::Put);
}

HttpRequest& HttpClient::del(const QString &path)
{
    return createRequest(path, HttpRequest::Method::Delete);
}

HttpRequest& HttpClient::createRequest(const QString &path, HttpRequest::Method method)
{
    HttpRequest *request = new HttpRequest(m_manager, m_baseUrl, path, method, this);
    // 添加默认头部
    for (auto it = m_defaultHeaders.begin(); it != m_defaultHeaders.end(); ++it)
        request->header(it.key(), it.value());
    return *request;
}
