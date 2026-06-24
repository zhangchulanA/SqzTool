#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QPointer>
#include <functional>
#include <QJsonArray>
#include "Logger.h"
/**
 * @brief HTTP 响应结构体，包含状态码、数据、错误信息等
 */
struct HttpResponse
{
    int statusCode = 0;                     ///< HTTP 状态码（200, 404, 500...）
    QByteArray rawData;                     ///< 原始响应字节流
    QJsonObject jsonObject;                 ///< 如果响应是 JSON 对象，则解析至此
    QJsonArray jsonArray;                   ///< 如果响应是 JSON 数组，则解析至此
    QString errorString;                    ///< 错误描述（来自 QNetworkReply）
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError; ///< Qt 网络错误码
    bool success = false;                   ///< 请求是否成功（状态码 2xx 且无网络错误）
};

/**
 * @brief 单个 HTTP 请求对象，支持链式配置与发送
 *
 * 典型用法：
 * @code
 * HttpClient client;
 * client.setBaseUrl("https://api.example.com");
 * client.get("/user")
 *       .param("id", 123)
 *       .header("Accept", "application/json")
 *       .timeout(5000)
 *       .success([](const HttpResponse& resp){
 *           qDebug() << resp.jsonObject;
 *       })
 *       .error([](const HttpResponse& resp){
 *           qDebug() << "Error:" << resp.errorString;
 *       })
 *       .send();
 * @endcode
 *
 * @note 所有方法（除 send() 外）均返回自身引用，支持链式调用。
 * @note 本对象为 HttpClient 的子对象，由 HttpClient 管理生命周期。
 * @note 非线程安全，请确保在创建 HttpClient 的线程中调用。
 */
class HttpRequest : public QObject
{
    Q_OBJECT

public:
    /// HTTP 方法枚举（使用 enum class 避免命名污染）
    enum class Method
    {
        Get,
        Post,
        Put,
        Delete
    };

    /// 成功回调类型：参数为 HttpResponse
    using SuccessCallback = std::function<void(const HttpResponse&)>;
    /// 错误回调类型：参数为 HttpResponse
    using ErrorCallback = std::function<void(const HttpResponse&)>;
    /// 进度回调类型：参数为已接收字节数、总字节数
    using ProgressCallback = std::function<void(qint64, qint64)>;
    /// 轮询继续判断回调：参数为最近一次响应，返回 true 表示继续轮询
    using PollContinueCallback = std::function<bool(const HttpResponse&)>;

public:
    /**
     * @brief 构造请求对象（仅供 HttpClient 调用）
     * @param manager 网络管理器（由 HttpClient 持有）
     * @param baseUrl 基础 URL
     * @param path    接口路径（可包含查询参数）
     * @param method  HTTP 方法
     * @param parent  父对象（通常为 HttpClient）
     */
    HttpRequest(QNetworkAccessManager *manager,
                const QString &baseUrl,
                const QString &path,
                Method method,
                QObject *parent = nullptr);

    /**
     * @brief 析构函数，取消正在进行的请求
     */
    ~HttpRequest();

    // ---------- 链式配置方法（均返回自身引用） ----------

    /**
     * @brief 添加自定义请求头
     * @param key   头部名称
     * @param value 头部值
     * @return 自身引用
     */
    HttpRequest& header(const QString &key, const QString &value);

    /**
     * @brief 添加 URL 查询参数（会与 path 中原有参数合并）
     * @param key   参数名
     * @param value 参数值
     * @return 自身引用
     */
    HttpRequest& param(const QString &key, const QVariant &value);

    /**
     * @brief 设置 JSON 请求体（Content-Type 自动设为 application/json）
     * @param json JSON 对象
     * @return 自身引用
     */
    HttpRequest& jsonBody(const QJsonObject &json);

    /**
     * @brief 设置表单请求体（Content-Type 自动设为 application/x-www-form-urlencoded）
     * @param form 键值对
     * @return 自身引用
     */
    HttpRequest& formBody(const QVariantMap &form);

    /**
     * @brief 设置原始请求体，并指定 Content-Type
     * @param data        原始字节数据
     * @param contentType MIME 类型
     * @return 自身引用
     */
    HttpRequest& rawBody(const QByteArray &data, const QString &contentType);

    /**
     * @brief 设置超时时间（毫秒）
     * @param ms 超时毫秒数，默认 10000
     * @return 自身引用
     */
    HttpRequest& timeout(int ms);

    /**
     * @brief 设置普通重试次数（在最终失败前重试，仅针对网络错误或非 2xx 状态码）
     * @param count 重试次数（不含首次请求），默认 0
     * @return 自身引用
     */
    HttpRequest& retry(int count);

    /**
     * @brief 设置轮询参数（在失败或满足条件时反复请求）
     * @param intervalMs    轮询间隔（毫秒）
     * @param maxAttempts   最大轮询尝试次数（-1 表示无限制），注意首次请求不计入轮询次数
     * @param shouldContinue 可选条件函数，每次响应后调用，返回 true 则继续轮询（无论成功与否），
     *                       若不提供，则仅当响应失败时继续轮询（兼容旧行为）
     * @return 自身引用
     */
    HttpRequest& polling(int intervalMs, int maxAttempts, PollContinueCallback shouldContinue = nullptr);

    /**
     * @brief 设置成功回调（仅在最终成功时触发）
     * @param cb 回调函数
     * @return 自身引用
     */
    HttpRequest& success(SuccessCallback cb);

    /**
     * @brief 设置错误回调（仅在最终失败时触发）
     * @param cb 回调函数
     * @return 自身引用
     */
    HttpRequest& error(ErrorCallback cb);

    /**
     * @brief 设置上传/下载进度回调
     * @param cb 回调函数，参数为已接收字节数和总字节数
     * @return 自身引用
     */
    HttpRequest& progress(ProgressCallback cb);

    /**
     * @brief 取消当前正在进行的请求（若已发送）
     * @note 取消后，会触发错误回调（errorCallback），且不再重试或轮询
     */
    void cancel();

    /**
     * @brief 发送请求（必须调用，链式最后一步）
     *
     * 发送后，请求对象存活直到最终回调执行完毕（或取消）。
     * 若重复调用 send()，则第二次调用无效（会打印警告）。
     */
    void send();

private:
    // ---------- 内部辅助函数 ----------
    void execute();                          // 真正执行一次网络请求
    QUrl buildUrl() const;                  // 构建完整 URL（合并参数）
    QNetworkRequest buildRequest() const;   // 构建 QNetworkRequest
    void handleReply(QNetworkReply *reply); // 处理网络响应
    void cleanup();                         // 清理资源并最终删除对象

    bool m_sent = false;                    // 是否已调用 send()
    QPointer<QNetworkReply> m_currentReply; // 当前活跃的 reply（用于取消）
    QTimer *m_timeoutTimer = nullptr;       // 超时定时器（绑定到 reply）

    // 配置项
    QNetworkAccessManager *m_manager = nullptr;
    QString m_baseUrl;
    QString m_path;
    Method m_method;
    QMap<QString, QString> m_headers;
    QVariantMap m_params;
    QByteArray m_body;
    QString m_contentType;
    int m_timeoutMs = 10000;
    int m_retryCount = 0;
    int m_currentRetry = 0;

    // 轮询配置
    int m_pollIntervalMs = 0;
    int m_maxPollAttempts = 0;
    int m_currentPollAttempt = 0;
    PollContinueCallback m_pollContinue;    // 自定义轮询条件

    // 回调
    SuccessCallback m_successCallback;
    ErrorCallback m_errorCallback;
    ProgressCallback m_progressCallback;

    // 状态标志
    bool m_cancelled = false;               // 是否已被取消
};

/**
 * @brief HTTP 客户端类，管理默认配置和请求对象的创建
 *
 * 使用方式：
 * @code
 * HttpClient client;
 * client.setBaseUrl("http://127.0.0.1:8080");
 * client.setBearerToken("your_token");
 * client.get("/users").param("page", 1).send();
 * @endcode
 *
 * @note 每个 HttpRequest 对象均为 HttpClient 的子对象，因此 HttpClient 析构时会自动删除所有请求。
 * @note 非线程安全，所有操作应在同一线程执行。
 */
class HttpClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit HttpClient(QObject *parent = nullptr);

    /**
     * @brief 设置基础 URL（例如 "https://api.example.com"）
     * @param baseUrl 基础 URL，末尾可带或不带 '/'
     */
    void setBaseUrl(const QString &baseUrl);

    /**
     * @brief 设置默认请求头（所有请求自动携带）
     * @param key   头部名称
     * @param value 头部值
     */
    void setDefaultHeader(const QString &key, const QString &value);

    /**
     * @brief 设置 Bearer Token（自动添加 Authorization: Bearer <token>）
     * @param token 令牌字符串
     */
    void setBearerToken(const QString &token);

    // ---------- 创建请求 ----------
    /**
     * @brief 创建 GET 请求
     * @param path 路径（可包含查询参数）
     * @return HttpRequest 引用，用于链式配置
     */
    HttpRequest& get(const QString &path);

    /**
     * @brief 创建 POST 请求
     * @param path 路径
     * @return HttpRequest 引用
     */
    HttpRequest& post(const QString &path);

    /**
     * @brief 创建 PUT 请求
     * @param path 路径
     * @return HttpRequest 引用
     */
    HttpRequest& put(const QString &path);

    /**
     * @brief 创建 DELETE 请求
     * @param path 路径
     * @return HttpRequest 引用
     */
    HttpRequest& del(const QString &path);

private:
    HttpRequest& createRequest(const QString &path, HttpRequest::Method method);

    QString m_baseUrl;
    QMap<QString, QString> m_defaultHeaders;
    QNetworkAccessManager *m_manager = nullptr;
};

#endif // HTTPCLIENT_H
