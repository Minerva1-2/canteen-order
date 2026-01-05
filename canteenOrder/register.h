#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "softkeyboard.h"

// 注意：C++中 register 是关键字，类名首字母必须大写
class Register : public QWidget
{
    Q_OBJECT
public:
    explicit Register(QWidget *parent = 0);

signals:
    void goBackToLogin();

private slots:
    void onRegisterClicked();
    void clearEdit();

private:
    QLineEdit *userEdit;
    QLineEdit *passEdit;
    QLineEdit *confirmPassEdit;
    QPushButton *btnRegister;
    QPushButton *btnCancel;
};

#endif // REGISTER_H
