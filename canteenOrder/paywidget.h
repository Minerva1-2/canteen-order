#ifndef PAYWIDGET_H
#define PAYWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include "hardwarecontrol.h"
#include "minimqtt.h"

class PayWidget : public QDialog
{
    Q_OBJECT
public:
    // 构造函数接收总金额，用于显示
    explicit PayWidget(int amount, QWidget *parent = nullptr);
    void setOrderData(const QString &json);

private:
    void initUI();
    void generateRandomQRCode(); // 模拟生成随机二维码

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    QLabel *lblAmount;
    QLabel *lblQRCode; // 用于显示二维码图片
    int m_amount;
    QString m_jsonOrder; // 存储订单JSON字符串
    MiniMqtt *m_mqtt;
};

#endif // PAYWIDGET_H
