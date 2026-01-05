#include "paywidget.h"
#include "hardwarecontrol.h" // [Important] Must include this header to use hardware control
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QTime>
#include <QDebug>

PayWidget::PayWidget(int amount, QWidget *parent) : QDialog(parent), m_amount(amount)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setFixedSize(360, 420);
    qsrand(QTime::currentTime().msec());
    initUI();

    // [1. 初始化 MQTT]
    m_mqtt = new MiniMqtt(this);

    // 连接成功时的调试信息
    connect(m_mqtt, &MiniMqtt::connected, [=](){
        qDebug() << "PayWidget: MQTT Connected!";
    });
    m_mqtt->connectToHost
            (MQTT_IP, MQTT_PORT);
}

void PayWidget::setOrderData(const QString &json)
{
    m_jsonOrder = json;
    qDebug() << "PayWidget received order data:" << m_jsonOrder;
}

void PayWidget::initUI()
{
    QWidget *bgWidget = new QWidget(this);
    bgWidget->setGeometry(0, 0, 360, 420);
    bgWidget->setStyleSheet("background-color: white; border-radius: 12px; border: 1px solid #DDD;");

    QVBoxLayout *mainLayout = new QVBoxLayout(bgWidget);
    mainLayout->setContentsMargins(20, 30, 20, 20);
    mainLayout->setSpacing(15);

    QLabel *lblTitle = new QLabel("扫码支付", bgWidget);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #333; border: none;");

    lblAmount = new QLabel(QStringLiteral("支付金额: %1 元").arg(m_amount), bgWidget);
    lblAmount->setAlignment(Qt::AlignCenter);
    lblAmount->setStyleSheet("font-size: 18px; color: #FF5339; font-weight: bold; border: none;");

    lblQRCode = new QLabel(bgWidget);
    lblQRCode->setFixedSize(200, 200);
    lblQRCode->setStyleSheet("border: 1px solid #EEE;");
    lblQRCode->setAlignment(Qt::AlignCenter);

    generateRandomQRCode();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    QPushButton *btnCancel = new QPushButton("取消支付", bgWidget);
    btnCancel->setFixedHeight(45);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet(
                "QPushButton { background-color: #F5F5F5; color: #666; font-size: 16px; border-radius: 22px; border: 1px solid #DDD; }"
                "QPushButton:pressed { background-color: #E0E0E0; }"
                );
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(onCancelClicked()));

    QPushButton *btnConfirm = new QPushButton("确认支付", bgWidget);
    btnConfirm->setFixedHeight(45);
    btnConfirm->setCursor(Qt::PointingHandCursor);
    btnConfirm->setStyleSheet(
                "QPushButton { background-color: #26C28D; color: white; font-size: 16px; border-radius: 22px; font-weight: bold; }"
                "QPushButton:pressed { background-color: #1E946A; }"
                );
    connect(btnConfirm, SIGNAL(clicked()), this, SLOT(onConfirmClicked()));

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnConfirm);

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblAmount);
    mainLayout->addWidget(lblQRCode, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void PayWidget::generateRandomQRCode()
{
    QPixmap pix(200, 200);
    pix.fill(Qt::white);

    QPainter painter(&pix);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    int gridSize = 25;
    int cellSize = 8;

    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            if ((i < 7 && j < 7) || (i < 7 && j > 17) || (i > 17 && j < 7)) {
                continue;
            }
            if (qrand() % 2 == 0) {
                painter.drawRect(i * cellSize, j * cellSize, cellSize, cellSize);
            }
        }
    }

    auto drawFinderPattern = [&](int x, int y) {
        painter.setBrush(Qt::black);
        painter.drawRect(x, y, 7*cellSize, 7*cellSize);
        painter.setBrush(Qt::white);
        painter.drawRect(x + cellSize, y + cellSize, 5*cellSize, 5*cellSize);
        painter.setBrush(Qt::black);
        painter.drawRect(x + 2*cellSize, y + 2*cellSize, 3*cellSize, 3*cellSize);
    };

    drawFinderPattern(0, 0);
    drawFinderPattern((gridSize - 7) * cellSize, 0);
    drawFinderPattern(0, (gridSize - 7) * cellSize);

    lblQRCode->setPixmap(pix);
}

void PayWidget::onConfirmClicked()
{
    HardwareControl::instance()->playSuccessSound();
    HardwareControl::instance()->flashLedSuccess();

    if (m_mqtt && !m_jsonOrder.isEmpty()) {
        m_mqtt->publish("canteen/order/new", m_jsonOrder);
        qDebug() << "MQTT Sending:" << m_jsonOrder;
        QThread::msleep(50);
    }

    accept();
}

void PayWidget::onCancelClicked()
{
    reject();
}
