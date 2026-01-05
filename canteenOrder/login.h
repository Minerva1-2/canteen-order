#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "softkeyboard.h"
#include "register.h"
#include "maininterface.h"

class login : public QWidget
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = 0);
    ~login();

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private slots:
    void onLoginClicked();
    void showRegisterPage();
    void showLoginPage();

private:
    QLineEdit *userEdit;
    QLineEdit *passEdit;
    QPushButton *btnLogin;
    QPushButton *btnToRegister;

    Register *registerPage; // 【修改】类名变为 Register
    QWidget *loginContainer;
    QVBoxLayout *mainLayout;

    SoftKeyboard *keyboard;
};

#endif // LOGIN_H
