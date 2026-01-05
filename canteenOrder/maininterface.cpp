#include "maininterface.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QCoreApplication>
#include <QPixmap>

MainInterface::MainInterface(QWidget *parent) : QWidget(parent)
{
    this->setFixedSize(800, 480);

    // 初始化指针
    headerWidget = nullptr;
    tabWidget = nullptr;
    stackedWidget = nullptr;
    m_orderPage = nullptr;
    m_videoPage = nullptr;
    m_settlePage = nullptr;

    this->setStyleSheet(
                "MainInterface { background-color: #FFFFFF; font-family: 'Microsoft YaHei'; }"
                );

    initUI();
}

void MainInterface::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    initHeader(); // 初始化头部和Tab

    // === 初始化 StackedWidget 和子页面 ===
    stackedWidget = new QStackedWidget(this);

    // 1. 点餐页
    m_orderPage = new OrderWidget(this);
    stackedWidget->addWidget(m_orderPage); // Index 0

    // 2. 视频页
    m_videoPage = new VideoWidget(this);
    connect(m_videoPage, SIGNAL(videoStarted()), this, SLOT(handleVideoStarted()));
    connect(m_videoPage, SIGNAL(videoStopped()), this, SLOT(handleVideoStopped()));
    stackedWidget->addWidget(m_videoPage); // Index 1

    // 3. 结算页 (SettleWidget)
    // 注意：这里假设 SettleWidget 就是你实现 MQTT 发送的那个界面
    m_settlePage = new SettleWidget(this);
    connect(m_settlePage, SIGNAL(paySuccess()), this, SLOT(handlePaySuccess()));
    stackedWidget->addWidget(m_settlePage); // Index 2

    // 4. 已点菜品页 (Index 3)
    m_haveOrderedPage = new HaveOrdered(this);
    stackedWidget->addWidget(m_haveOrderedPage);

    // 连接 Stack 切换信号
    connect(stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(onPageChanged(int)));

    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(stackedWidget);

    // 默认显示第一页
    onPageChanged(0);
}


void MainInterface::initHeader()
{
    headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(90);
    headerWidget->setStyleSheet("background-color: #2D2D2D;");

    QHBoxLayout *hLayout = new QHBoxLayout(headerWidget);
    hLayout->setContentsMargins(15, 10, 15, 0);

    QLabel *imgLabel = new QLabel(headerWidget);
    imgLabel->setFixedSize(70, 70);
    QPixmap pixmap(":/res/logo.png");
    if (pixmap.isNull()) {
        // 图片加载失败
        imgLabel->setText("LOGO");
        imgLabel->setStyleSheet("background-color: #FF8C00; color: white; border-radius: 8px; font-weight: bold; qproperty-alignment: AlignCenter;");
        qDebug() << "警告: 图片加载失败，请检查路径";
    } else {
        // 图片加载成功
        QPixmap scaledPix = pixmap.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        imgLabel->setPixmap(scaledPix);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setStyleSheet("background-color: transparent; border-radius: 8px;");
    }

    QVBoxLayout *infoLayout = new QVBoxLayout();
    QLabel *nameLabel = new QLabel("美滋滋寿司餐厅", headerWidget);
    nameLabel->setStyleSheet("color: white; font-size: 22px; font-weight: bold;");
    QLabel *detailLabel = new QLabel("★★★★★ 4.9分 | 配送 5 元", headerWidget);
    detailLabel->setStyleSheet("color: #AAAAAA; font-size: 12px;");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(detailLabel);

    hLayout->addWidget(imgLabel);
    hLayout->addLayout(infoLayout);
    hLayout->addStretch();

    // 导航 Tab 栏
    tabWidget = new QWidget(this);
    tabWidget->setFixedHeight(60);
    tabWidget->setStyleSheet("background-color: white; border-bottom: 1px solid #E0E0E0;");

    QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);

    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(30);

    auto createBtn = [=](const QString &text, const QString &objName, int index) {
        QPushButton *btn = new QPushButton(text, tabWidget);
        btn->setObjectName(objName);
        btn->setCheckable(true);
        btn->setFixedWidth(120);
        btn->setStyleSheet(
                    "QPushButton { border: none; font-size: 16px; color: #666; background: transparent; padding-bottom: 10px; margin-top: 10px; }"
                    "QPushButton:checked { color: #333; font-weight: bold; border-bottom: 3px solid #FFD161; }"
                    );
        connect(btn, &QPushButton::clicked, [=](){
            if(stackedWidget) stackedWidget->setCurrentIndex(index);
            });
        return btn;
        };

    QPushButton *btnOrder = createBtn("餐厅点餐", "btnOrder", 0);
    QPushButton *btnVideo = createBtn("菜品介绍", "btnVideo", 1);
    QPushButton *btnCheckout = createBtn("确认下单", "btnCheckout", 2);
    QPushButton *btnHaveordered = createBtn("已点菜品", "btnHaveordered", 3);

    tabLayout->addSpacing(20);
    tabLayout->addWidget(btnOrder);
    tabLayout->addWidget(btnVideo);
    tabLayout->addWidget(btnCheckout);
    tabLayout->addWidget(btnHaveordered);
    tabLayout->addStretch();
}

void MainInterface::onPageChanged(int index)
{
    // 1. 处理 Tab 按钮的高亮状态
    if(tabWidget) {
        QList<QPushButton*> btns = tabWidget->findChildren<QPushButton*>();
        for(auto btn : btns) {
            btn->blockSignals(true);
            btn->setChecked(false); // 先全灭
        }
        QPushButton *target = nullptr;

        if(index == 0) target = tabWidget->findChild<QPushButton*>("btnOrder");
        if(index == 1) target = tabWidget->findChild<QPushButton*>("btnVideo");
        if(index == 2) target = tabWidget->findChild<QPushButton*>("btnCheckout");
        if(index == 3) target = tabWidget->findChild<QPushButton*>("btnHaveordered");

        if(target) target->setChecked(true);

        for(auto btn : btns) btn->blockSignals(false);
    }

    // 2. 特殊逻辑：如果切换到了“确认下单”页 (Index 2)
    if (index == 2) {
        if (m_orderPage && m_settlePage) {
            // A. 同步购物车数据和价格 (用于界面显示)
            QMap<QString, int> cart = m_orderPage->getCartData();
            QMap<QString, double> prices = m_orderPage->getPriceData();

            qDebug() << "Syncing data to SettlePage. Cart items:" << cart.size();
            m_settlePage->updateOrderInfo(cart, prices);
            QString mqttJson = m_orderPage->getOrderJson();
            m_settlePage->setOrderData(mqttJson);
        }
    }
    if (index != 1 && m_videoPage) {
        m_videoPage->stopVideo();
    }
}

void MainInterface::handleVideoStarted()
{
    if(headerWidget) headerWidget->hide();
    if(tabWidget) tabWidget->hide();
}

void MainInterface::handleVideoStopped()
{
    if(headerWidget) headerWidget->show();
    if(tabWidget) tabWidget->show();
}

void MainInterface::handlePaySuccess()
{
    // 1. 将订单数据移动到“已点菜品”历史记录
    if (m_orderPage && m_haveOrderedPage) {
        // 获取购物车数据
        QMap<QString, int> cart = m_orderPage->getCartData();
        if (!cart.isEmpty()) {
            m_haveOrderedPage->addOrder(cart);
        }
    }

    // 2. 清空购物车 (为下一次点单做准备)
    if (m_orderPage) {
        m_orderPage->clearCart();
    }

    // 3. 支付成功后自动跳转回点餐首页
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(0);
    }
}
