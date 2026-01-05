#include "login.h"
#include "dbmanager.h"
#include <QMessageBox>
#include <QEvent>

login::login(QWidget *parent) : QWidget(parent)
{
    this->setFixedSize(800, 480);
    this->setWindowFlags(Qt::FramelessWindowHint);

    mainLayout = new QVBoxLayout(this);

    // --- 1. 登录容器 ---
    loginContainer = new QWidget(this);
    QVBoxLayout *loginLayout = new QVBoxLayout(loginContainer);
    loginLayout->setContentsMargins(200, 50, 200, 50);
    loginLayout->setSpacing(20);

    QLabel *logoLabel = new QLabel("美味餐厅点单系统", this);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setObjectName("Title");

    userEdit = new QLineEdit(this);
    userEdit->setPlaceholderText("用户名");

    passEdit = new QLineEdit(this);
    passEdit->setPlaceholderText("密码");
    passEdit->setEchoMode(QLineEdit::Password);

    btnLogin = new QPushButton("登 录", this);
    btnToRegister = new QPushButton("注册新账号", this);
    btnToRegister->setObjectName("SecondaryButton");

    loginLayout->addStretch();
    loginLayout->addWidget(logoLabel);
    loginLayout->addWidget(userEdit);
    loginLayout->addWidget(passEdit);
    loginLayout->addWidget(btnLogin);
    loginLayout->addWidget(btnToRegister);
    loginLayout->addStretch();

    // --- 2. 注册页面 ---
    // 【修改】实例化 Register 类
    registerPage = new Register(this);
    registerPage->hide();

    mainLayout->addWidget(loginContainer);
    mainLayout->addWidget(registerPage);

    // --- 3. 信号槽 ---
    connect(btnLogin, SIGNAL(clicked()), this, SLOT(onLoginClicked()));
    connect(btnToRegister, SIGNAL(clicked()), this, SLOT(showRegisterPage()));
    connect(registerPage, SIGNAL(goBackToLogin()), this, SLOT(showLoginPage()));

    // --- 4. 键盘 ---
    keyboard = new SoftKeyboard(this);
    keyboard->hide();

    userEdit->installEventFilter(this);
    passEdit->installEventFilter(this);
}

login::~login() {}

void login::onLoginClicked()
{
    QString name = userEdit->text();
    QString pass = passEdit->text();

    // 验证用户名和密码
    if(DBManager::instance().loginUser(name, pass)){
        // 1. 先把软键盘收起来
        SoftKeyboard::instance()->hide();

        // 3. 创建主界面对象
        MainInterface *mainWin = new MainInterface();

        // 4. 显示主界面
        mainWin->showFullScreen();
        this->close();
    } else {
        QMessageBox::warning(this, "错误", "用户名或密码错误");
        name.clear();
        pass.clear();
    }
}

void login::showRegisterPage()
{
    loginContainer->hide();
    registerPage->show();
}

void login::showLoginPage()
{
    registerPage->hide();
    loginContainer->show();
}

bool login::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::FocusIn)
    {
        if (watched == userEdit || watched == passEdit) {
            keyboard->move(0, this->height() - keyboard->height());
            keyboard->raise();
            keyboard->show();
        }
    }
    return QWidget::eventFilter(watched, event);
}
