#ifndef CANDATAPARSER_H
#define CANDATAPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QString>

// 单个信号的配置结构体
struct SignalConfig
{
    QString name;        // 信号名
    int startByte;       // 起始字节
    int startBit;        // 起始位
    int bitLength;       // 位长度
    double factor;       // 缩放因子
    double offset;       // 偏移量
    QString unit;        // 单位
    QString endianness;  // 大小端(Little/BIG)
    QMap<int, QString> statusMap; // Status节点映射(值-描述)
};

// CAN消息配置结构体
struct CanMsgConfig
{
    QString name;                // 消息名(如发动机/变速器)
    int dlc;                     // 数据长度
    QMap<QString, SignalConfig> msignals; // 信号集合(信号名-配置)
};

class CanDataParser : public QObject
{
    Q_OBJECT
public:
    explicit CanDataParser(QObject *parent = nullptr);

    // 加载XML配置文件(返回是否成功)
    bool loadConfig(const QString& xmlPath);
    // 加载XML配置内容(适用于内存中的XML字符串)
    bool loadConfigContent(const QString& xmlContent);
    // 解析CAN数据(核心接口)
    QJsonObject parseCanData(uint32_t canid, const QByteArray& canData);

private:
    // 解析XML配置的核心方法
    void parseXml(const QDomDocument& doc);
    // 从字节数组中解析单个信号的值
    double parseSignalValue(const QByteArray& data, const SignalConfig& config);
    // 解析二进制数据为数值(处理大小端和位提取)
    uint64_t getRawValue(const QByteArray& data, int startByte, int startBit, int bitLength, bool isLittleEndian);

private:
    QMap<uint32_t, CanMsgConfig> m_canConfigMap; // CANID-消息配置映射
};

#endif // CANDATAPARSER_H
