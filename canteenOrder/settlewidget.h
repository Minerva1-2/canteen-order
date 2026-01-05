#ifndef SETTLEWIDGET_H
#define SETTLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include "minimqtt.h"

class SettleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettleWidget(QWidget *parent = nullptr);

    // 核心功能：接收外部传来的数据并刷新界面
    void updateOrderInfo(const QMap<QString, int> &cart, const QMap<QString, double> &prices);
    void setOrderData(const QString &json);

signals:
    void paySuccess(); // 支付成功信号，通知主界面

private slots:
    void onPayClicked();

private:
    void initUI();

private:
    QTableWidget *tableCart;
    QLabel *lblFinalPrice;
    QPushButton *btnConfirmPay;
    int currentTotalPrice;
    QString m_jsonOrder;  // 存储从 MainInterface 传来的 JSON
        MiniMqtt *m_mqtt;
};

#endif // SETTLEWIDGET_H
