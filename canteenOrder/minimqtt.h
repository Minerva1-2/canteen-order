#ifndef MINIMQTT_H
#define MINIMQTT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class MiniMqtt : public QObject
{
    Q_OBJECT
public:
    explicit MiniMqtt(QObject *parent = nullptr);

    // 连接到服务器
    void connectToHost(const QString &host, quint16 port);
    // 发布消息
    void publish(const QString &topic, const QString &message);
    // 订阅主题
    void subscribe(const QString &topic);

signals:
    void connected();
    void received(const QString &topic, const QString &message);
    void disconnected();

private slots:
    void onSocketConnected();
    void onSocketReadyRead();

private:
    QTcpSocket *m_socket;
    QByteArray encodeRemainingLength(int len);
    QByteArray encodeString(const QString &str);
};

#endif // MINIMQTT_H
