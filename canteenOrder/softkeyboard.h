#ifndef SOFTKEYBOARD_H
#define SOFTKEYBOARD_H

#include <QDialog>
#include <QPushButton>
#include <QList>
#include <QEvent>
#include <QLineEdit>

class SoftKeyboard : public QDialog
{
    Q_OBJECT
public:
    // 单例模式获取实例
    static SoftKeyboard* instance();
    explicit SoftKeyboard(QWidget *parent = 0);

protected:
    // 事件过滤器：监听输入框点击
    bool eventFilter(QObject *watched, QEvent *event);

private:
    static SoftKeyboard* m_instance;

    void initUI();
    void updateKeyLabels();

private slots:
    void onButtonClicked();

private:
    // UI 组件
    QList<QPushButton*> m_letterButtons; // 存储字母/数字键引用
    QPushButton *btnShift;
    QPushButton *btnMode; // 123/ABC 切换键

    // 状态标志
    bool isCapital; // 是否大写
    bool isNumber;  // 是否数字模式
};

#endif // SOFTKEYBOARD_H
