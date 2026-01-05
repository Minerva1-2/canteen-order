#ifndef HARDWARECONTROL_H
#define HARDWARECONTROL_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QFile>
#include <QDebug>

// 按键监听线程类
class KeyMonitorThread : public QThread
{
    Q_OBJECT
public:
    volatile unsigned int* gpio_base = nullptr;
    bool stopFlag = false;

protected:
    void run() override;

signals:
    void urgeKeyPressed(); // 线程发出的原始信号
};

// 硬件控制主类
class HardwareControl : public QObject
{
    Q_OBJECT
public:
    static HardwareControl* instance(); // 单例获取
    void initHardware();                // 初始化

    // 硬件动作接口
    void flashLedSuccess();    // LED 闪烁
    void playSuccessSound();   // 蜂鸣器响

private:
    explicit HardwareControl(QObject *parent = nullptr);
    static HardwareControl* m_instance;

    // 内存映射指针
    volatile unsigned int* gpioc_base; // 蜂鸣器
    volatile unsigned int* gpiob_base; // 按键

    // 辅助对象
    KeyMonitorThread *keyThread;
    QTimer *beepTimer;
    int beepDuration = 0;
    bool beepState = false;

signals:
    void urgeOrderTriggered(); // 【关键】转发给 UI 的信号

private slots:
    void stopLed();
    void toggleBeep();
};

#endif // HARDWARECONTROL_H
