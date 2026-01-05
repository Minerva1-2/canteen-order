#include "settlewidget.h"
#include "paywidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>

SettleWidget::SettleWidget(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("QWidget { background-color: #F0F2F5; }");
    currentTotalPrice = 0;
    initUI();
    m_mqtt = new MiniMqtt(this);

    // 打印连接状态
    connect(m_mqtt, &MiniMqtt::connected, [=](){
        qDebug() << "SettlePage: MQTT Connected!";
    });
    m_mqtt->connectToHost(MQTT_IP, MQTT_PORT);
}

void SettleWidget::initUI()
{
    QHBoxLayout *checkoutLayout = new QHBoxLayout(this);
    checkoutLayout->setContentsMargins(20, 20, 20, 20);
    checkoutLayout->setSpacing(20);

    // 1. 左侧：账单
    QWidget *billWidget = new QWidget(this);
    billWidget->setStyleSheet("background-color: white; border-radius: 10px;");
    QVBoxLayout *billLayout = new QVBoxLayout(billWidget);
    billLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *lblBillTitle = new QLabel(QStringLiteral("购物小票"), billWidget);
    lblBillTitle->setAlignment(Qt::AlignCenter);
    lblBillTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #333; padding-bottom: 10px; border-bottom: 2px dashed #EEE;");

    tableCart = new QTableWidget(billWidget);
    tableCart->setColumnCount(3);
    tableCart->setHorizontalHeaderLabels(QStringList() << QStringLiteral("菜品") << QStringLiteral("数量") << QStringLiteral("金额"));
    tableCart->verticalHeader()->setVisible(false);
    tableCart->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableCart->setSelectionMode(QAbstractItemView::NoSelection);
    tableCart->setFocusPolicy(Qt::NoFocus);
    tableCart->setStyleSheet(
                "QTableWidget { border: none; gridline-color: #F0F0F0; background: white; }"
                "QHeaderView::section { background: white; color: #999; border: none; font-weight: bold; font-size: 14px; }"
                );
    tableCart->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tableCart->setColumnWidth(1, 60);
    tableCart->setColumnWidth(2, 80);

    billLayout->addWidget(lblBillTitle);
    billLayout->addWidget(tableCart);

    // 2. 右侧：支付面板
    QWidget *payControlWidget = new QWidget(this);
    payControlWidget->setFixedWidth(280);
    payControlWidget->setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *payLayout = new QVBoxLayout(payControlWidget);
    payLayout->setContentsMargins(20, 40, 20, 40);
    payLayout->setSpacing(30);
    payLayout->addStretch();

    QLabel *lblTotalTitle = new QLabel(QStringLiteral("应付总额"), payControlWidget);
    lblTotalTitle->setAlignment(Qt::AlignCenter);
    lblTotalTitle->setStyleSheet("color: #666; font-size: 18px;");

    lblFinalPrice = new QLabel(QStringLiteral("0 元"), payControlWidget);
    lblFinalPrice->setAlignment(Qt::AlignCenter);
    lblFinalPrice->setStyleSheet("color: #FF5339; font-size: 48px; font-weight: bold; font-family: 'Arial';");

    btnConfirmPay = new QPushButton(QStringLiteral("立即支付"), payControlWidget);
    btnConfirmPay->setFixedHeight(60);
    btnConfirmPay->setCursor(Qt::PointingHandCursor);
    btnConfirmPay->setStyleSheet(
                "QPushButton { background-color: #26C28D; color: white; font-size: 20px; font-weight: bold; border-radius: 30px; border: 2px solid #20A576; }"
                "QPushButton:pressed { background-color: #1E946A; border-color: #1E946A; }"
                );
    connect(btnConfirmPay, SIGNAL(clicked()), this, SLOT(onPayClicked()));

    payLayout->addWidget(lblTotalTitle);
    payLayout->addWidget(lblFinalPrice);
    payLayout->addWidget(btnConfirmPay);
    payLayout->addStretch();

    checkoutLayout->addWidget(billWidget, 1);
    checkoutLayout->addWidget(payControlWidget);
}

void SettleWidget::updateOrderInfo(const QMap<QString, int> &cart, const QMap<QString, double> &prices)
{
    tableCart->setRowCount(0);
    currentTotalPrice = 0;

    qDebug() << "--- Sync Start ---";
    QMapIterator<QString, int> i(cart);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        int count = i.value();

        // 核心修复逻辑：先转 int 再计算
        int unitPrice = 0;
        if (prices.contains(name)) {
            unitPrice = (int)prices.value(name);
        } else {
            // 如果名字没找到，尝试打印出来对比
            qDebug() << "Warning: Price missing for:" << name;
        }

        int subTotal = unitPrice * count;
        currentTotalPrice += subTotal;

        qDebug() << "Item:" << name << " Price:" << unitPrice << " Total:" << subTotal;

        int row = tableCart->rowCount();
        tableCart->insertRow(row);

        QTableWidgetItem *item0 = new QTableWidgetItem(name);
        item0->setFont(QFont("Microsoft YaHei", 12));
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableCart->setItem(row, 0, item0);

        QTableWidgetItem *item1 = new QTableWidgetItem(QString("x %1").arg(count));
        item1->setTextAlignment(Qt::AlignCenter);
        tableCart->setItem(row, 1, item1);

        QTableWidgetItem *item2 = new QTableWidgetItem(QStringLiteral("%1 元").arg(subTotal));
        item2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item2->setFont(QFont("Arial", 12, QFont::Bold));
        tableCart->setItem(row, 2, item2);
    }

    if(cart.isEmpty()) {
        tableCart->setRowCount(1);
        QTableWidgetItem *emptyItem = new QTableWidgetItem(QStringLiteral("您的购物车空空如也"));
        emptyItem->setTextAlignment(Qt::AlignCenter);
        tableCart->setItem(0, 0, emptyItem);
        tableCart->setSpan(0, 0, 1, 3);
    }

    lblFinalPrice->setText(QStringLiteral("%1 元").arg(currentTotalPrice));
}

void SettleWidget::onPayClicked()
{
    if (currentTotalPrice <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("购物车是空的，快去选购心仪的美食吧！"));
        return;
    }

    PayWidget payDialog(currentTotalPrice, this);

    if (payDialog.exec() == QDialog::Accepted) {
        QMessageBox::information(this, QStringLiteral("支付成功"),
                                 QStringLiteral("支付成功！\n共消费 %1 元。\n\n正在为您制作美食，请稍候...").arg(currentTotalPrice));

        currentTotalPrice = 0;
        lblFinalPrice->setText(QStringLiteral("0 元"));
        tableCart->setRowCount(0);

        if (m_mqtt && !m_jsonOrder.isEmpty()) {
            m_mqtt->publish("canteen/order/new", m_jsonOrder);
            qDebug() << "MQTT Published: " << m_jsonOrder;
        } else {
            qDebug() << "Error: MQTT not ready or JSON is empty";
        }

        emit paySuccess();
    }
    else {
        qDebug() << "Payment Cancelled";
    }
}

void SettleWidget::setOrderData(const QString &json)
{
    m_jsonOrder = json;
    qDebug() << "SettlePage received JSON:" << m_jsonOrder;
}
