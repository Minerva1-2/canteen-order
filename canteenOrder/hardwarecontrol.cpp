#include "hardwarecontrol.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

// === 硬件路径定义 (LED) ===
// GEC6818 的 LED 子系统路径
#define PATH_LED_BRIGHTNESS "/sys/class/leds/led1/brightness"
// 如果 led1 不亮，尝试改为 led2, led3, led4

// === S5P6818 物理寄存器地址 (蜂鸣器 & 按键) ===
#define GPIOB_PHY_BASE 0xC001B000 // 按键 (GPIOB30)
#define GPIOC_PHY_BASE 0xC001C000 // 蜂鸣器 (PWM2 -> GPIOC14 复用)

// 寄存器偏移
#define GPIO_OUT     0x00
#define GPIO_OUTENB  0x04
#define GPIO_PAD     0x18

// 引脚号
#define KEY_PIN      30   // GPIOB30
#define BEEP_PIN     14   // GPIOC14 (PWM2引脚)

HardwareControl* HardwareControl::m_instance = nullptr;

HardwareControl* HardwareControl::instance()
{
    if (m_instance == nullptr) {
        m_instance = new HardwareControl();
    }
    return m_instance;
}

HardwareControl::HardwareControl(QObject *parent) : QObject(parent)
{
    gpioc_base = nullptr;
    gpiob_base = nullptr;

    // 按键线程
    keyThread = new KeyMonitorThread();
    connect(keyThread, &KeyMonitorThread::urgeKeyPressed, this, &HardwareControl::urgeOrderTriggered);

    // 蜂鸣器定时器 (用于产生音频频率)
    beepTimer = new QTimer(this);
    connect(beepTimer, &QTimer::timeout, this, &HardwareControl::toggleBeep);
}

// 内存映射通用函数
volatile unsigned int* map_register(off_t target_base) {
    int mem_fd = ::open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        qDebug() << "Error: Failed to open /dev/mem. Need Root permission!";
        return nullptr;
    }

    void *map_base = ::mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, target_base);
    ::close(mem_fd);

    if (map_base == MAP_FAILED) {
        qDebug() << "Error: mmap failed.";
        return nullptr;
    }
    return (volatile unsigned int *)map_base;
}

// 辅助函数：写文件 (用于 LED)
void writeSysfs(const QString &path, const QString &val) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(val.toLatin1());
        file.close();
    } else {
        qDebug() << "Sysfs Write Error:" << path;
    }
}

void HardwareControl::initHardware()
{
    // 1. 初始化 LED (Sysfs)
    // 既然系统已经有了 /sys/class/leds/led1，我们直接用，不需要初始化
    // 先尝试关灯
    writeSysfs(PATH_LED_BRIGHTNESS, "0");
    qDebug() << "LED initialized via Sysfs (led1)";

    // 2. 初始化 蜂鸣器 (Mmap -> GPIOC14)
    gpioc_base = map_register(GPIOC_PHY_BASE);
    if (gpioc_base) {
        // 设置 GPIOC14 为输出 (OUTENB 第14位置1)
        *(gpioc_base + (GPIO_OUTENB >> 2)) |= (1 << BEEP_PIN);
        // 初始电平拉低 (关)
        *(gpioc_base + (GPIO_OUT >> 2)) &= ~(1 << BEEP_PIN);
        qDebug() << "Beep initialized via Mmap (GPIOC14)";
    }

    // 3. 初始化 按键 (Mmap -> GPIOB30)
    gpiob_base = map_register(GPIOB_PHY_BASE);
    if (gpiob_base) {
        // 设置 GPIOB30 为输入 (OUTENB 第30位置0)
        *(gpiob_base + (GPIO_OUTENB >> 2)) &= ~(1 << KEY_PIN);

        // 启动线程
        keyThread->gpio_base = gpiob_base;
        keyThread->start();
        qDebug() << "Button initialized via Mmap (GPIOB30)";
    }
}

// --- LED 控制 (Sysfs) ---
void HardwareControl::flashLedSuccess()
{
    // 写入 1 (或者 255) 点亮
    // 注意：有些板子 1 是亮，有些 0 是亮，请根据实际情况调整
    writeSysfs(PATH_LED_BRIGHTNESS, "255"); // 尝试最大亮度

    // 200ms 后关闭
    QTimer::singleShot(3000, this, SLOT(stopLed()));
}

void HardwareControl::stopLed()
{
    writeSysfs(PATH_LED_BRIGHTNESS, "0");
}

// --- 蜂鸣器控制 (软件 PWM) ---
void HardwareControl::playSuccessSound()
{
    if (!gpioc_base) return;

    beepDuration = 0;
    // 1ms 翻转一次，约 500Hz
    beepTimer->start(1);
}

void HardwareControl::toggleBeep()
{
    if (!gpioc_base) return;

    beepState = !beepState;
    if (beepState) {
        *(gpioc_base + (GPIO_OUT >> 2)) |= (1 << BEEP_PIN);
    } else {
        *(gpioc_base + (GPIO_OUT >> 2)) &= ~(1 << BEEP_PIN);
    }

    // 响 150ms (150次翻转)
    if (++beepDuration > 150) {
        beepTimer->stop();
        // 强制拉低，确保停止发声
        *(gpioc_base + (GPIO_OUT >> 2)) &= ~(1 << BEEP_PIN);
    }
}

// --- 按键监听线程 ---
void KeyMonitorThread::run()
{
    if (!gpio_base) return;

    int lastState = 1; // 默认高电平

    while (!stopFlag) {
        // 读取 GPIO_PAD 状态
        unsigned int val = *(gpio_base + (GPIO_PAD >> 2));
        // 提取第 30 位
        int currentState = (val >> KEY_PIN) & 0x1;

        // 下降沿检测 (1 -> 0)
        if (lastState == 1 && currentState == 0) {
            qDebug() << "Physical Button Pressed!";
            emit urgeKeyPressed();
            QThread::msleep(300); // 消抖
        }
        lastState = currentState;
        QThread::msleep(50); // 轮询间隔
    }
}
