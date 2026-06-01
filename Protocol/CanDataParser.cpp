#include "CanDataParser.h"
#include <QFile>
#include <QDomElement>
//#include <QDomParseError>
#include <QDomNode>
#include <QDebug>
#include <QJsonValue>
#include <QDebug>
#define log (qDebug()<<"["<<__LINE__<<__FUNCTION__<<"]")

CanDataParser::CanDataParser(QObject *parent) : QObject(parent)
{
}

bool CanDataParser::loadConfig(const QString &xmlPath)
{
    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "打开CAN配置文件失败:" << file.errorString();
        return false;
    }
    // 读取文件内容并调用内存解析接口
    QString xmlContent = file.readAll();
    file.close();
    return loadConfigContent(xmlContent);
}

bool CanDataParser::loadConfigContent(const QString &xmlContent)
{
    QDomDocument doc;
//    QDomParseError error;
    if (!doc.setContent(xmlContent))
    {
        qWarning() << "XML配置解析失败:" ;
        return false;
    }
    // 清空原有配置，重新解析
    m_canConfigMap.clear();
    parseXml(doc);
    return !m_canConfigMap.isEmpty();
}

void CanDataParser::parseXml(const QDomDocument &doc)
{
    QDomElement root = doc.documentElement();
    if (root.tagName() != "CANDATA")
    {
        qWarning() << "XML根节点不是CANDATA，解析失败";
        return;
    }
    // 遍历所有Message节点
    QDomNodeList msgNodes = root.elementsByTagName("Message");
    for (int i = 0; i < msgNodes.count(); i++)
    {
        QDomElement msgElem = msgNodes.at(i).toElement();
        // 解析CANID(字符串转无符号整型)
        uint32_t canid = msgElem.attribute("canid").toUInt();
        // 解析消息基本信息
        CanMsgConfig msgConfig;
        msgConfig.name = msgElem.attribute("name");
        msgConfig.dlc = msgElem.attribute("dlc").toInt();

        // 遍历所有DATA1信号节点
        QDomNodeList signalNodes = msgElem.elementsByTagName("DATA1");
        for (int j = 0; j < signalNodes.count(); j++)
        {
            QDomElement sigElem = signalNodes.at(j).toElement();
            SignalConfig sigConfig;
            // 解析信号基础配置
            sigConfig.name = sigElem.attribute("name");
            sigConfig.startByte = sigElem.attribute("startByte").toInt();
            sigConfig.startBit = sigElem.attribute("startBit").toInt();
            sigConfig.bitLength = sigElem.attribute("bitLength").toInt();
            sigConfig.factor = sigElem.attribute("factor").toDouble();
            sigConfig.offset = sigElem.attribute("offset").toDouble();
            sigConfig.unit = sigElem.attribute("unit");
            sigConfig.endianness = sigElem.attribute("endianness");

            // 解析Status子节点(故障状态映射)
            QDomNodeList statusNodes = sigElem.elementsByTagName("Status");
            for (int k = 0; k < statusNodes.count(); k++)
            {
                QDomElement statusElem = statusNodes.at(k).toElement();
                int value = statusElem.attribute("value").toInt();
                QString desc = statusElem.attribute("desc");
                sigConfig.statusMap[value] = desc;
            }

            // 将信号配置加入消息配置
            msgConfig.msignals[sigConfig.name] = sigConfig;
        }
        // 将消息配置加入全局映射
        m_canConfigMap[canid] = msgConfig;
    }
}

uint64_t CanDataParser::getRawValue(const QByteArray &data, int startByte, int startBit, int bitLength, bool isLittleEndian)
{
    // 边界检查：起始字节+位超出数据长度则返回0
    if (startByte >= data.size()) return 0;
    if (startBit < 0 || startBit > 7) return 0;
    if (bitLength < 1 || bitLength > 64) return 0;

    uint64_t rawValue = 0;
    int currentByte = startByte;
    int currentBit = startBit;

    // 逐位提取二进制数据
         log<<isLittleEndian<<data;
    for (int i = 0; i < bitLength; i++)
    {
        if (currentByte >= data.size()) break;
        // 提取当前位的值(0/1)
        uchar byteVal = static_cast<uchar>(data.at(currentByte));
        int bitVal = (byteVal >> currentBit) & 0x01;
        // 将位值写入结果(小端/大端仅影响位拼接顺序，硬件标准)

        if (isLittleEndian)
            rawValue |= (static_cast<uint64_t>(bitVal) << i);
        else
            rawValue = (rawValue << 1) | bitVal;


        // 位索引自增，超出7则切换到下一个字节
        currentBit++;
        if (currentBit > 7)
        {
            currentBit = 0;
            currentByte++;
        }
    }
    return rawValue;
}

double CanDataParser::parseSignalValue(const QByteArray &data, const SignalConfig &config)
{
    bool isLittle = (config.endianness.compare("Little", Qt::CaseInsensitive) == 0);
    // 提取原始二进制值
    uint64_t raw = getRawValue(data, config.startByte, config.startBit, config.bitLength, isLittle);
    // 计算实际值：原始值*缩放因子+偏移量
    return raw * config.factor + config.offset;
}

QJsonObject CanDataParser::parseCanData(uint32_t canid, const QByteArray &canData)
{
    QJsonObject resultObj;
    // 检查CANID是否在配置中
    if (!m_canConfigMap.contains(canid))
    {
        qWarning() << "未找到CANID:" << canid << "的配置信息";
        return resultObj;
    }

    // 加入canid到结果JSON
    resultObj["canid"] = QString::number(canid);
    CanMsgConfig msgConfig = m_canConfigMap[canid];

    // 遍历所有信号，逐个解析
    for (const SignalConfig& sigConfig : msgConfig.msignals)
    {
        QJsonObject sigObj;
        if (sigConfig.statusMap.isEmpty())
        {
            // 无Status节点：解析数值+单位
            double value = parseSignalValue(canData, sigConfig);
            sigObj["value"] = value;
            sigObj["unit"] = sigConfig.unit;
        }
        else
        {
            // 有Status节点：解析为状态描述(如故障/无故障)
            double rawValue = parseSignalValue(canData, sigConfig);
            int intValue = static_cast<int>(rawValue);
            // 查找状态描述，无匹配则返回原始值
            QString statusDesc = sigConfig.statusMap.value(intValue, QString::number(intValue));
            sigObj["value"] = statusDesc;
        }
        // 将信号对象加入结果(键为信号名)
        resultObj[sigConfig.name] = sigObj;
    }

    return resultObj;
}
