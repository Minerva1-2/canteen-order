#include "orderwidget.h"
#include "hardwarecontrol.h"
#include "paywidget.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScroller>
#include <QDebug>
#include <QRegExp>

OrderWidget::OrderWidget(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("background-color: #FFFFFF;");

    m_isOrderCompleted = false;
    m_haveOrderedPage = new HaveOrdered(this);
    m_haveOrderedPage->hide();
    m_mqtt = new MiniMqtt(this);

    initUI();
    updateDishList(QStringLiteral("热销榜"));

    connect(HardwareControl::instance(), &HardwareControl::urgeOrderTriggered,
            this, &OrderWidget::handleUrgeOrder);
    connect(m_mqtt, &MiniMqtt::connected, this, [=](){
        m_mqtt->subscribe("canteen/kitchen/status");
    });

    // 处理收到消息
    connect(m_mqtt, &MiniMqtt::connected, [=](){
        qDebug() << "OrderWidget: MQTT Connected!";
        // 订阅通知主题
        m_mqtt->subscribe("canteen/service/notify");
    });
    connect(m_mqtt, &MiniMqtt::received, this, [=](QString topic, QString message){

        // 过滤主题
        if(topic == "canteen/service/notify") {
            qDebug() << "Received Notification:" << message;
            bool isForMe = false;

            if (message.contains("notify") && (message.contains("\"table\":1") || message.contains("\"table\": 1"))) {
                isForMe = true;
            }
            if (isForMe) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("取餐提醒");
                msgBox.setText("您的餐点已经准备好！\n请前往柜台取餐。");
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setStandardButtons(QMessageBox::Ok);

                msgBox.setStyleSheet("QLabel{font-size: 20px; font-weight: bold;} QPushButton{width: 100px; height: 40px; font-size: 18px;}");

                msgBox.exec();
            }
        }
    });

    m_mqtt->connectToHost(MQTT_IP, MQTT_PORT);
}

void OrderWidget::initUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 左侧分类列表
    listCategories = new QListWidget(this);
    listCategories->setFixedWidth(150);
    listCategories->setFocusPolicy(Qt::NoFocus);
    listCategories->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(listCategories, QScroller::LeftMouseButtonGesture);
    listCategories->setIconSize(QSize(32, 32));
    listCategories->setStyleSheet(
                "QListWidget { background-color: #F2F4F7; border: none; outline: none; }"
                "QListWidget::item { height: 65px; padding-left: 5px; color: #666; font-size: 14px; border-bottom: 1px solid #E5E5E5; }"
                "QListWidget::item:selected { background-color: #FFFFFF; color: #333; font-weight: bold; border-left: 4px solid #FFD161; }"
                );

    addCategory(QStringLiteral("热销榜"),   ":/res/hot.png");
    addCategory(QStringLiteral("优惠套餐"), ":/res/preferential.png");
    addCategory(QStringLiteral("精品寿司"), ":/res/sushi.png");
    addCategory(QStringLiteral("日式拉面"), ":/res/romen.png");
    addCategory(QStringLiteral("手作饭团"), ":/res/rice.png");
    addCategory(QStringLiteral("新鲜刺身"), ":/res/cishen.png");
    addCategory(QStringLiteral("特色饮品"), ":/res/drink_logo.png");

    listCategories->setCurrentRow(0);
    connect(listCategories, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onCategoryClicked(QListWidgetItem*)));

    // 2. 右侧菜品列表
    listDishes = new QListWidget(this);
    listDishes->setFocusPolicy(Qt::NoFocus);
    listDishes->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listDishes->setStyleSheet("QListWidget { background-color: #FFFFFF; border: none; outline: none; }");
    listDishes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(listDishes, QScroller::LeftMouseButtonGesture);

    mainLayout->addWidget(listCategories);
    mainLayout->addWidget(listDishes);
}

void OrderWidget::addCategory(const QString &name, const QString &iconPath)
{
    QListWidgetItem *item = new QListWidgetItem(listCategories);
    if (!iconPath.isEmpty()) item->setIcon(QIcon(iconPath));
    item->setText(name);
}

void OrderWidget::onCategoryClicked(QListWidgetItem *item)
{
    if(item) updateDishList(item->text());
}

void OrderWidget::updateDishList(const QString &category)
{
    listDishes->clear();

    // 【关键】判断字符串时也使用 QStringLiteral
    if (category.contains(QStringLiteral("热销榜"))) {
        addDishItem(QStringLiteral("蛋丝三文鱼包饭"), "49", ":/res/sushi1.png", 214);
        addDishItem(QStringLiteral("招牌鳗鱼饭"), "58", ":/res/sushi2.png", 500);
        addDishItem(QStringLiteral("加州卷(4粒)"), "22", ":/res/sushi3.png", 120);
        addDishItem(QStringLiteral("铁火卷"), "667", ":/res/sushi5.png", 73);
        addDishItem(QStringLiteral("卷寿司"), "345", ":/res/sushi6.png", 37);
        addDishItem(QStringLiteral("豪华刺身拼盘"), "128", ":/res/sashimi1.png", 50);
        addDishItem(QStringLiteral("火腿海苔饭团"), "057", ":/res/rice2.png", 90);
        addDishItem(QStringLiteral("饭团拼盘"), "282", ":/res/rice3.png", 87);
    }
    else if (category.contains(QStringLiteral("优惠套餐"))) {
        addDishItem(QStringLiteral("稻荷寿司"), "257", ":/res/sushi8.png", 78);
        addDishItem(QStringLiteral("茶巾寿司"), "267", ":/res/sushi9.png", 89);
         addDishItem(QStringLiteral("经典日式豚骨拉面"), "672", ":/res/romen1.png", 120);
         addDishItem(QStringLiteral("经典刺身"), "355", ":/res/sashimi1.png", 120);
         addDishItem(QStringLiteral("刺身大拼盘"), "456", ":/res/sashimi2.png", 90);
         addDishItem(QStringLiteral("新鲜三文鱼刺身"), "980", ":/res/sashimi3.png", 87);
    }
    else if(category.contains(QStringLiteral("精品寿司"))){
        addDishItem(QStringLiteral("握寿司"), "232", ":/res/sushi1.png", 120);
        addDishItem(QStringLiteral("加州卷(4粒)"), "244", ":/res/sushi2.png", 90);
        addDishItem(QStringLiteral("军舰卷"), "654", ":/res/sushi3.png", 87);
        addDishItem(QStringLiteral("散寿司"), "436", ":/res/sushi4.png", 69);
        addDishItem(QStringLiteral("铁火卷"), "667", ":/res/sushi5.png", 73);
        addDishItem(QStringLiteral("卷寿司"), "345", ":/res/sushi6.png", 37);
        addDishItem(QStringLiteral("太卷"), "279", ":/res/sushi7.png", 34);
        addDishItem(QStringLiteral("稻荷寿司"), "257", ":/res/sushi8.png", 78);
        addDishItem(QStringLiteral("茶巾寿司"), "267", ":/res/sushi9.png", 89);
        addDishItem(QStringLiteral("押寿司"), "837", ":/res/sushi10.png", 27);
        addDishItem(QStringLiteral("手卷"), "282", ":/res/sushi11.png", 82);
        addDishItem(QStringLiteral("奶油三文鱼寿司"), "134", ":/res/sushi12.png", 81);
    }
    else if(category.contains(QStringLiteral("日式拉面"))){
        addDishItem(QStringLiteral("经典日式豚骨拉面"), "672", ":/res/romen1.png", 120);
        addDishItem(QStringLiteral("日式牛肉拉面"), "365", ":/res/romen2.png", 90);
        addDishItem(QStringLiteral("日式清汤拉面"), "568", ":/res/romen3.png", 87);
    }
    else if(category.contains(QStringLiteral("手作饭团"))){
        addDishItem(QStringLiteral("豆芽饭团"), "353", ":/res/rice1.png", 120);
        addDishItem(QStringLiteral("火腿海苔饭团"), "057", ":/res/rice2.png", 90);
        addDishItem(QStringLiteral("饭团拼盘"), "282", ":/res/rice3.png", 87);
    }
    else if(category.contains(QStringLiteral("新鲜刺身"))){
        addDishItem(QStringLiteral("经典刺身"), "355", ":/res/sashimi1.png", 120);
        addDishItem(QStringLiteral("刺身大拼盘"), "456", ":/res/sashimi2.png", 90);
        addDishItem(QStringLiteral("新鲜三文鱼刺身"), "980", ":/res/sashimi3.png", 87);
    }
    else if(category.contains(QStringLiteral("特色饮品"))){
        addDishItem(QStringLiteral("经典可乐"), "273", ":/res/drink.png", 120);
        addDishItem(QStringLiteral("日本清酒"), "183", ":/res/drink2.png", 90);
        addDishItem(QStringLiteral("鲜榨葡萄汁"), "641", ":/res/drink3.png", 87);
    }
}

void OrderWidget::addDishItem(const QString &name, const QString &priceStr, const QString &imagePath, int sales)
{
    bool ok;
    int priceVal = priceStr.trimmed().toInt(&ok);

    if (!ok) {
        qDebug() << "Price Error for" << name << ":" << priceStr;
        priceVal = 0;
    }

    // 更新价格表
    m_prices.insert(name, priceVal);

    // --- 界面绘制 ---
    QWidget *itemWidget = new QWidget(this);
    itemWidget->setFixedHeight(125);
    itemWidget->setStyleSheet("background-color: white; border-bottom: 1px solid #F0F0F0;");

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);

    QLabel *imgLabel = new QLabel(itemWidget);
    imgLabel->setFixedSize(80, 80);
    imgLabel->setStyleSheet("border: 1px solid #EEE; border-radius: 4px;");
    if(!imagePath.isEmpty()) {
        QPixmap pix(imagePath);
        if(!pix.isNull()){
            imgLabel->setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            imgLabel->setScaledContents(true);
        } else {
            imgLabel->setText(QStringLiteral("无图"));
            imgLabel->setAlignment(Qt::AlignCenter);
        }
    }

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->setContentsMargins(0, 2, 0, 2);

    QLabel *lblName = new QLabel(name, itemWidget);
    lblName->setStyleSheet("font-size: 16px; color: #222; border: none; font-weight: bold;");
    lblName->setWordWrap(true);

    QLabel *lblDesc = new QLabel(QStringLiteral("主厨推荐 | 现点现做"), itemWidget);
    lblDesc->setStyleSheet("font-size: 12px; color: #888; border: none;");

    QLabel *lblSales = new QLabel(QStringLiteral("月售 %1").arg(sales), itemWidget);
    lblSales->setStyleSheet("font-size: 11px; color: #999; border: none;");

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(4);

    QLabel *lblPriceNum = new QLabel(priceStr, itemWidget);
    lblPriceNum->setStyleSheet("font-size: 24px; color: #FF4D4F; font-weight: bold; border: none; font-family: 'Arial';");

    QLabel *lblUnit = new QLabel(QStringLiteral("元"), itemWidget);
    lblUnit->setStyleSheet("font-size: 14px; color: #FF4D4F; padding-bottom: 3px; border: none;");

    QPushButton *btnMinus = new QPushButton("－", itemWidget);
    btnMinus->setFixedSize(28, 28);
    btnMinus->setStyleSheet(
                "QPushButton { border: 1px solid #DDD; background: white; color: #888; font-size: 18px; border-radius: 14px; font-weight: bold; }"
                "QPushButton:pressed { background: #EEE; }"
                );

    int currentCount = m_cart.value(name, 0);
    QLabel *lblCount = new QLabel(QString::number(currentCount), itemWidget);
    lblCount->setFixedWidth(30);
    lblCount->setAlignment(Qt::AlignCenter);
    lblCount->setStyleSheet("color: #333; font-size: 16px; border: none; font-weight: bold;");

    QPushButton *btnAdd = new QPushButton("＋", itemWidget);
    btnAdd->setFixedSize(28, 28);
    btnAdd->setStyleSheet(
                "QPushButton { border: none; background-color: #FFD161; color: #333; font-size: 18px; border-radius: 14px; font-weight: bold; }"
                "QPushButton:pressed { background-color: #FBC02D; }"
                );

    connect(btnAdd, &QPushButton::clicked, [=](){
        int count = m_cart.value(name, 0);
        count++;
        m_cart.insert(name, count);
        lblCount->setText(QString::number(count));
        emit cartUpdated(m_cart.size());
    });

    connect(btnMinus, &QPushButton::clicked, [=](){
        int count = m_cart.value(name, 0);
        if(count > 0) {
            count--;
            if(count == 0) m_cart.remove(name);
            else m_cart.insert(name, count);
            lblCount->setText(QString::number(count));
            emit cartUpdated(m_cart.size());
        }
    });

    bottomLayout->addWidget(lblPriceNum, 0, Qt::AlignBottom);
    bottomLayout->addWidget(lblUnit, 0, Qt::AlignBottom);
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnMinus);
    bottomLayout->addWidget(lblCount);
    bottomLayout->addWidget(btnAdd);

    infoLayout->addWidget(lblName);
    infoLayout->addWidget(lblDesc);
    infoLayout->addWidget(lblSales);
    infoLayout->addStretch();
    infoLayout->addLayout(bottomLayout);

    layout->addWidget(imgLabel);
    layout->addLayout(infoLayout);

    QListWidgetItem *item = new QListWidgetItem(listDishes);
    item->setSizeHint(QSize(400, 125));
    listDishes->setItemWidget(item, itemWidget);
}

void OrderWidget::clearCart()
{
    m_cart.clear();
    m_isOrderCompleted = false;
    QListWidgetItem *currentItem = listCategories->currentItem();
    if(currentItem) updateDishList(currentItem->text());
    else updateDishList(QStringLiteral("热销榜"));
}

void OrderWidget::setOrderCompleted(bool completed)
{
    m_isOrderCompleted = completed;

    if(completed) {
        qDebug() << "Order Payment Success. Transferring data to History...";
        // 检查指针有效性
        if (m_haveOrderedPage) {
            // 遍历当前购物车，将每一项加入历史记录
            QMapIterator<QString, int> i(m_cart);
            while (i.hasNext()) {
                i.next();
                QString name = i.key();
                int count = i.value();
                // 获取该菜品单价
                int price = m_prices.value(name, 0);
                m_haveOrderedPage->addOrderedItem(name, count, price);
            }
            // 清空当前购物车
            clearCart();
            emit cartUpdated(0);
        }
    }
}
void OrderWidget::handleUrgeOrder()
{
    qDebug() << "Hardware Button Signal Received.";
    if (!canUrgeOrder()) {
        qDebug() << "Urge Ignored: No paid orders found.";
        return;
    }

    HardwareControl::instance()->flashLedSuccess();
    HardwareControl::instance()->playSuccessSound();

    QMessageBox box(QMessageBox::Information, QStringLiteral("催单成功"),
                    QStringLiteral("我们要加急了！\n厨师正在飞速制作中！"));
    box.setStandardButtons(QMessageBox::Ok);
    box.setButtonText(QMessageBox::Ok, QStringLiteral("知道了"));

    QTimer::singleShot(2000, &box, SLOT(accept()));
    box.exec();
}

void OrderWidget::processPaymentSuccess()
{
    // 1. 遍历购物车，将菜品移动到“已点菜品”列表
    QMapIterator<QString, int> i(m_cart);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        int count = i.value();
        int price = m_prices.value(name); // 从价格表中获取单价

        m_haveOrderedPage->addOrderedItem(name, count, price);
    }

    // 2. 清空当前购物车
    m_cart.clear();
    m_haveOrderedPage->show();
}

void OrderWidget::showHaveOrderedWindow()
{
    if(m_haveOrderedPage) {
        m_haveOrderedPage->show();
    }
}

QString OrderWidget::getOrderJson()
{
    int totalAmount = 0;
    QString jsonItems;
    bool firstItem = true;

    // 遍历购物车构建 JSON
    QMapIterator<QString, int> i(m_cart);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        int count = i.value();
        int price = m_prices.value(name, 0);
        totalAmount += (price * count);

        // 手动拼接 JSON 数组项
        if (!firstItem) jsonItems += ",";
        jsonItems += QString("{\"name\":\"%1\",\"count\":%2,\"price\":%3}")
                .arg(name)
                .arg(count)
                .arg(price);
        firstItem = false;
    }

    // 组合最终 JSON
    QString finalJson = QString("{\"table\":1, \"total\":%1, \"items\":[%2]}")
            .arg(totalAmount)
            .arg(jsonItems);

    return finalJson;
}
