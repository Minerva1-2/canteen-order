#include "register.h"
#include "dbmanager.h"
#include <QMessageBox>

Register::Register(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(100, 30, 100, 30);
    mainLayout->setSpacing(20);

    QLabel *title = new QLabel("新用户注册", this);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName("Title");

    userEdit = new QLineEdit(this);
    userEdit->setPlaceholderText("请输入用户名");
    userEdit->setFixedHeight(50);

    passEdit = new QLineEdit(this);
    passEdit->setPlaceholderText("设置密码");
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setFixedHeight(50);

    confirmPassEdit = new QLineEdit(this);
    confirmPassEdit->setPlaceholderText("确认密码");
    confirmPassEdit->setEchoMode(QLineEdit::Password);
    confirmPassEdit->setFixedHeight(50);

    btnRegister = new QPushButton("确认注册", this);
    btnCancel = new QPushButton("返回登录", this);
    btnCancel->setObjectName("SecondaryButton");

    mainLayout->addStretch();
    mainLayout->addWidget(title);
    mainLayout->addWidget(userEdit);
    mainLayout->addWidget(passEdit);
    mainLayout->addWidget(confirmPassEdit);
    mainLayout->addWidget(btnRegister);
    mainLayout->addWidget(btnCancel);
    mainLayout->addStretch();

    connect(btnRegister, SIGNAL(clicked()), this, SLOT(onRegisterClicked()));
    connect(btnCancel, SIGNAL(clicked()), this, SIGNAL(goBackToLogin()));

    userEdit->installEventFilter(SoftKeyboard::instance());
    passEdit->installEventFilter(SoftKeyboard::instance());
    confirmPassEdit->installEventFilter(SoftKeyboard::instance());
}

void Register::clearEdit()
{
    userEdit->clear();
    passEdit->clear();
    confirmPassEdit->clear();
}

void Register::onRegisterClicked()
{
    QString name = userEdit->text();
    QString pass = passEdit->text();
    QString confirm = confirmPassEdit->text();

    if(name.isEmpty() || pass.isEmpty()){
        QMessageBox::warning(this, "提示", "用户名或密码不能为空");
        userEdit->clear();
        passEdit->clear();
        confirmPassEdit->clear();
        return;
    }

    if(pass != confirm){
        QMessageBox::warning(this, "提示", "两次输入的密码不一致");
        clearEdit();
        return;
    }

    SoftKeyboard::instance()->hide();

    // 调用数据库保存
    if(DBManager::instance().registerUser(name, pass)){
        QMessageBox::information(this, "成功", "注册成功！");
        clearEdit();

        emit goBackToLogin();
    } else {
        QMessageBox::warning(this, "失败", "注册失败，用户名可能已存在");
        clearEdit();
    }
}
