#ifndef HAVEORDERED_H
#define HAVEORDERED_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include <QPushButton>
#include "minimqtt.h"


class HaveOrdered : public QWidget
{
    Q_OBJECT
public:
    explicit HaveOrdered(QWidget *parent = nullptr);

    // 接收支付成功的订单数据
    void addOrder(const QMap<QString, int> &cart);
    // 单项添加接口
    void addOrderedItem(const QString &name, int count, int price);
    // 判断是否有订单
    bool hasOrders() const;

private:
    void initUI();
    void updateHeaderInfo(); // 更新顶部统计信息

private slots:
    // 处理硬件催单信号
    void handleUrge();

private:
    QListWidget *listOrders; // 列表
    QLabel *lblTitle;        // 顶部标题
    QLabel *lblCountInfo;    // 顶部数量统计
    QLabel *lblStatus;       // 底部状态栏
    QMap<QString, int> m_totalOrderedItems; // 数据源
    QPushButton *m_btnUrge; // 催单按钮
    MiniMqtt *m_mqtt;
};

#endif // HAVEORDERED_H
