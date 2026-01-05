#include "minimqtt.h"
#include <QDebug>
#include <QTime>

MiniMqtt::MiniMqtt(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MiniMqtt::onSocketConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MiniMqtt::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MiniMqtt::disconnected);

    qsrand(QTime::currentTime().msec());
}

void MiniMqtt::connectToHost(const QString &host, quint16 port)
{
    if(m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_socket->connectToHost(host, port);
}

void MiniMqtt::onSocketConnected()
{
    // 构建 MQTT CONNECT 报文 (协议版本 3.1.1)
    QByteArray variableHeader;
    variableHeader.append((char)0x00); variableHeader.append((char)0x04); // 协议名长度
    variableHeader.append("MQTT");      // 协议名
    variableHeader.append((char)0x04);  // 协议级别 (3.1.1)
    variableHeader.append((char)0x02);  // 连接标志 (Clean Session)
    variableHeader.append((char)0x00); variableHeader.append((char)0x3C); // Keep Alive (60秒)

    QString clientId = "GEC6818_" + QString::number(qrand() % 10000);
    QByteArray payload = encodeString(clientId); // Client ID

    QByteArray fixedHeader;
    fixedHeader.append((char)0x10); // 报文类型: CONNECT
    fixedHeader.append(encodeRemainingLength(variableHeader.size() + payload.size()));

    m_socket->write(fixedHeader + variableHeader + payload);
}

void MiniMqtt::publish(const QString &topic, const QString &message)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[MQTT Error] Cannot publish, socket not connected.";
        return;
    }

    // 构建 PUBLISH 报文 (QoS 0)
    QByteArray topicBytes = encodeString(topic);
    QByteArray msgBytes = message.toUtf8();

    QByteArray fixedHeader;
    fixedHeader.append((char)0x30); // 报文类型: PUBLISH
    fixedHeader.append(encodeRemainingLength(topicBytes.size() + msgBytes.size()));

    m_socket->write(fixedHeader + topicBytes + msgBytes);
    m_socket->flush();
    qDebug() << "[MQTT] Published to" << topic << "(Size:" << msgBytes.size() << ")";
}

void MiniMqtt::subscribe(const QString &topic)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    // 构建 SUBSCRIBE 报文 (QoS 0)
    QByteArray variableHeader;
    variableHeader.append((char)0x00); variableHeader.append((char)0x01); // Packet Identifier (1)

    QByteArray payload = encodeString(topic);
    payload.append((char)0x00); // Requested QoS (0)

    QByteArray fixedHeader;
    fixedHeader.append((char)0x82); // 报文类型: SUBSCRIBE
    fixedHeader.append(encodeRemainingLength(variableHeader.size() + payload.size()));

    m_socket->write(fixedHeader + variableHeader + payload);
    qDebug() << "[MQTT] Subscribed to" << topic;
}

void MiniMqtt::onSocketReadyRead()
{
    QByteArray data = m_socket->readAll();

    // 简单的报文解析 (仅处理 CONNACK 和 PUBLISH)
    if (data.size() < 2) return;

    unsigned char type = (unsigned char)data[0] & 0xF0;

    if (type == 0x20) { // CONNACK
        if ((unsigned char)data[3] == 0x00) {
            qDebug() << "[MQTT] Connected Successfully!";
            emit connected();
        }
    }
    else if (type == 0x30) { // PUBLISH
        int remainingLength = (unsigned char)data[1];
        int varHeaderIndex = 2;
        if(remainingLength > 127) {
            return;
        }

        // 解析 Topic
        int topicLen = (unsigned char)data[varHeaderIndex] * 256 + (unsigned char)data[varHeaderIndex+1];
        QString topic = QString::fromUtf8(data.mid(varHeaderIndex + 2, topicLen));

        // 解析 Payload
        QByteArray payload = data.mid(varHeaderIndex + 2 + topicLen);
        QString message = QString::fromUtf8(payload);

        emit received(topic, message);
        qDebug() << "[MQTT] Received:" << topic << message;
    }
}

QByteArray MiniMqtt::encodeRemainingLength(int len)
{
    QByteArray bytes;
    do {
        char digit = len % 128;
        len /= 128;
        if (len > 0) digit |= 0x80;
        bytes.append(digit);
    } while (len > 0);
    return bytes;
}

QByteArray MiniMqtt::encodeString(const QString &str)
{
    QByteArray raw = str.toUtf8();
    QByteArray bytes;
    bytes.append((char)(raw.size() >> 8));
    bytes.append((char)(raw.size() & 0xFF));
    bytes.append(raw);
    return bytes;
}
