#ifndef ORDERWIDGET_H
#define ORDERWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QMap>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "haveordered.h"
#include "minimqtt.h"

class OrderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OrderWidget(QWidget *parent = nullptr);

    // 提供给外部获取数据的接口
    QMap<QString, int> getCartData() const { return m_cart; }
    QMap<QString, double> getPriceData() const { return m_prices; }
    void clearCart(); // 支付完成后清空购物车
    void showHaveOrderedWindow();
    void processPaymentSuccess();
    bool canUrgeOrder() { return m_haveOrderedPage && m_haveOrderedPage->hasOrders(); }
    QString getOrderJson();

signals:
    void cartUpdated(int totalCount); // 购物车变化信号（可选，用于更新主页红点等）

private:
    void initUI();
    void addCategory(const QString &name, const QString &iconPath);
    void updateDishList(const QString &category);
    void addDishItem(const QString &name, const QString &priceStr, const QString &imagePath, int sales);
    void updateTotalPrice(); // 内部计算逻辑
    void setOrderCompleted(bool completed);

private slots:
    void onCategoryClicked(QListWidgetItem *item);
    void handleUrgeOrder();


private:
    QListWidget *listCategories;
    QListWidget *listDishes;

    // 数据成员
    QMap<QString, int> m_cart;      // <菜名, 数量>
    QMap<QString, double> m_prices; // <菜名, 单价>
    bool m_isOrderCompleted;
    HaveOrdered *m_haveOrderedPage;
    MiniMqtt *m_mqtt;
};

#endif // ORDERWIDGET_H
