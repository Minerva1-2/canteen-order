#include "haveordered.h"
#include "hardwarecontrol.h"
#include <QHBoxLayout>
#include <QDebug>
#include <QScroller>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>

HaveOrdered::HaveOrdered(QWidget *parent) : QWidget(parent)
{
    this->setAttribute(Qt::WA_StyledBackground);
    this->setStyleSheet("background-color: #F5F5F5;"); // 全局浅灰背景，突出内容卡片感

    initUI();

    m_mqtt = new MiniMqtt(this);
    connect(m_mqtt, &MiniMqtt::connected, [=](){
        qDebug() << "HaveOrdered: MQTT Connected!";
    });
    m_mqtt->connectToHost(MQTT_IP, MQTT_PORT);
}

void HaveOrdered::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === 1. 顶部 Header 区域 (橙色背景) ===
    QWidget *headerContainer = new QWidget(this);
    headerContainer->setFixedHeight(60);
    headerContainer->setStyleSheet("background-color: #FF8C00;"); // 品牌橙色

    QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    lblTitle = new QLabel("历史订单", this);
    lblTitle->setStyleSheet("color: white; font-size: 20px; font-weight: bold; font-family: 'Microsoft YaHei';");

    lblCountInfo = new QLabel("共 0 道菜品", this);
    lblCountInfo->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 16px;");

    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(lblCountInfo);

    // === 2. 中间列表区域 ===
    listOrders = new QListWidget(this);
    listOrders->setFocusPolicy(Qt::NoFocus); // 去除选中虚线框
    listOrders->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // 平滑滚动
    listOrders->setSelectionMode(QAbstractItemView::NoSelection); // 禁止选中高亮

    // 支持触摸滑动
    QScroller::grabGesture(listOrders, QScroller::LeftMouseButtonGesture);

    // 列表美化样式
    listOrders->setStyleSheet(
                "QListWidget {"
                "   background-color: white;"
                "   border: none;"
                "   outline: none;"
                "}"
                "QListWidget::item {"
                "   height: 60px;"                 // 增加行高，方便查看
                "   border-bottom: 1px solid #EEE;" // 优雅的分割线
                "   padding-left: 20px;"
                "   padding-right: 20px;"
                "   color: #333;"
                "   font-size: 18px;"
                "}"
                );

    // === 3. 底部状态栏 (白色悬浮感) ===
    QWidget *bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(50);
    bottomBar->setStyleSheet("background-color: white; border-top: 1px solid #DDD;");

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 0, 20, 0);

    // 加一个小图标或者圆点装饰
    QLabel *dotLabel = new QLabel("●", this);
    dotLabel->setStyleSheet("color: #26C28D; font-size: 14px;"); // 绿色圆点

    lblStatus = new QLabel("当前无进行中的订单", this);
    lblStatus->setStyleSheet("color: #666; font-size: 16px; font-weight: bold;");

    bottomLayout->addWidget(dotLabel);
    bottomLayout->addSpacing(10);
    bottomLayout->addWidget(lblStatus);
    bottomLayout->addStretch();

    // === 组装布局 ===
    mainLayout->addWidget(headerContainer);
    mainLayout->addWidget(listOrders); // 列表占据主要空间
    mainLayout->addWidget(bottomBar);

    // 连接硬件信号
    connect(HardwareControl::instance(), &HardwareControl::urgeOrderTriggered,
            this, &HaveOrdered::handleUrge);
}

void HaveOrdered::updateHeaderInfo()
{
    int count = m_totalOrderedItems.size();
    lblCountInfo->setText(QStringLiteral("共 %1 道菜品").arg(count));

    if (count > 0) {
        lblStatus->setText(QStringLiteral("后厨正在加紧制作中，请耐心等待..."));
    } else {
        lblStatus->setText(QStringLiteral("当前无进行中的订单"));
    }
}

void HaveOrdered::addOrder(const QMap<QString, int> &cart)
{
    if (cart.isEmpty()) return;

    QMapIterator<QString, int> i(cart);
    while (i.hasNext()) {
        i.next();
        QString dishName = i.key();
        int count = i.value();

        if (m_totalOrderedItems.contains(dishName)) {
            m_totalOrderedItems[dishName] += count;
        } else {
            m_totalOrderedItems.insert(dishName, count);
        }
    }

    // 重新渲染列表
    listOrders->clear();
    QMapIterator<QString, int> j(m_totalOrderedItems);
    while (j.hasNext()) {
        j.next();

        // 使用 HTML 格式化文本，让菜名和数量对齐更漂亮
        // 菜名在左，数量加粗在右
        QString itemHtml = QString(
                    "<div style='display: flex; justify-content: space-between;'>"
                    "<span style='font-weight:bold;'>%1</span>"
                    "<span style='color:#FF8C00;'>x %2</span>"
                    "</div>"
                    ).arg(j.key()).arg(j.value());

        // 纯文本回退方案（QListWidget 默认不支持复杂的 HTML 布局，我们用空格模拟简单对齐）
        QString itemText = QStringLiteral("%1").arg(j.key());
        QString countText = QStringLiteral("x %1").arg(j.value());

        // 创建 Item
        QListWidgetItem *item = new QListWidgetItem();
        // 这里为了简单美观，我们将直接设置文本，利用 QListWidget 的样式
        item->setText(QStringLiteral("%1      %2").arg(j.key()).arg(countText));

        // 设置图标 (如果有资源可以加 icon)
        // item->setIcon(QIcon(":/images/food.png"));

        listOrders->addItem(item);
    }

    listOrders->scrollToBottom();
    updateHeaderInfo();
    qDebug() << "History Updated via Cart.";
}

void HaveOrdered::addOrderedItem(const QString &name, int count, int price)
{
    if (m_totalOrderedItems.contains(name)) {
        m_totalOrderedItems[name] += count;
    } else {
        m_totalOrderedItems.insert(name, count);
    }

    // 显示格式： 菜名  x 数量  (价格)
    QString displayText = QStringLiteral("%1      x %2      (￥%3)")
            .arg(name)
            .arg(count)
            .arg(price);

    QListWidgetItem *item = new QListWidgetItem(displayText);
    listOrders->addItem(item);
    listOrders->scrollToBottom();

    updateHeaderInfo();
    qDebug() << "Item added direct:" << name;
}

void HaveOrdered::handleUrge()
{
    if (m_totalOrderedItems.isEmpty()) return;

    QString jsonCmd = "{\"type\":\"service\", \"action\":\"urge\", \"table\":1}";

    m_mqtt->publish("canteen/service/urge", jsonCmd);
    qDebug() << "Urge sent:" << jsonCmd;

    HardwareControl::instance()->flashLedSuccess();
    HardwareControl::instance()->playSuccessSound();

    lblStatus->setText(QStringLiteral("已发送催单提醒！厨师收到了！"));
    lblStatus->setStyleSheet("color: #FF0000; font-size: 16px; font-weight: bold;");

    QTimer::singleShot(3000, this, [=](){
        lblStatus->setText(QStringLiteral("后厨正在加紧制作中，请耐心等待..."));
        lblStatus->setStyleSheet("color: #666; font-size: 16px; font-weight: bold;");
    });

    // 弹窗提示
    QMessageBox *box = new QMessageBox(QMessageBox::Information,
                                       QStringLiteral("催单成功"),
                                       QStringLiteral("您的催单请求已发送！\n服务员马上就来！"),
                                       QMessageBox::Ok, this);
    QTimer::singleShot(2000, box, SLOT(close()));
    box->show();
}

bool HaveOrdered::hasOrders() const
{
    return !m_totalOrderedItems.isEmpty();
}
