#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include "orderwidget.h"
#include "videowidget.h"
#include "settlewidget.h"
#include "haveordered.h"
#include "minimqtt.h"

class MainInterface : public QWidget
{
    Q_OBJECT

public:
    explicit MainInterface(QWidget *parent = nullptr);

private:
    void initUI();
    void initHeader(); // 头部 Logo 和 Tab

private slots:
    void onPageChanged(int index);   // 页面切换逻辑

    // 响应子模块的信号
    void handleVideoStarted();
    void handleVideoStopped();
    void handlePaySuccess();

private:
    // 布局容器
    QWidget *headerWidget;
    QWidget *tabWidget;
    QStackedWidget *stackedWidget;

    // 三大子模块实例
    OrderWidget *m_orderPage;
    VideoWidget *m_videoPage;
    SettleWidget *m_settlePage;
    HaveOrdered *m_haveOrderedPage;
};

#endif // MAININTERFACE_H
