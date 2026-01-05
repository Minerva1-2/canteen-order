#include "softkeyboard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

// 静态成员初始化
SoftKeyboard* SoftKeyboard::m_instance = nullptr;

// 字符映射表
const QStringList CHARS_LOWER = QStringList() << "q"<<"w"<<"e"<<"r"<<"t"<<"y"<<"u"<<"i"<<"o"<<"p"<<"a"<<"s"<<"d"<<"f"<<"g"<<"h"<<"j"<<"k"<<"l"<<"z"<<"x"<<"c"<<"v"<<"b"<<"n"<<"m";
const QStringList CHARS_UPPER = QStringList() << "Q"<<"W"<<"E"<<"R"<<"T"<<"Y"<<"U"<<"I"<<"O"<<"P"<<"A"<<"S"<<"D"<<"F"<<"G"<<"H"<<"J"<<"K"<<"L"<<"Z"<<"X"<<"C"<<"V"<<"B"<<"N"<<"M";
const QStringList CHARS_SYMBOL= QStringList() << "1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8"<<"9"<<"0"<<"-"<<"/"<<":"<<";"<<"("<<")"<<"$"<<"&&"<<"@"<<"\""<<"."<<","<<"?"<<"!"<<"'"<<"_";

SoftKeyboard* SoftKeyboard::instance() {
    if (!m_instance) m_instance = new SoftKeyboard();
    return m_instance;
}

SoftKeyboard::SoftKeyboard(QWidget *parent) : QDialog(parent) {
    // 初始化状态
    isCapital = false;
    isNumber = false;

    // 无边框窗口，始终在最顶层
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    this->setFixedSize(800, 260);

    // 【修改点 1】背景色：深咖啡色底座 (像木质托盘)
    this->setStyleSheet(
        "SoftKeyboard { "
        "   background-color: #4E342E; " // 深咖啡色
        "   border-top: 4px solid #8D6E63; " // 顶部亮一点的咖啡色边框
        "}"
    );

    initUI();
}

void SoftKeyboard::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6); // 稍微增加间距
    mainLayout->setContentsMargins(10, 15, 10, 10);

    // 【修改点 2】普通按键样式：奶油白/米白色 (像白巧克力)
    QString btnStyle =
            "QPushButton { "
            "   background-color: #FFF8E1; " // 米白色
            "   color: #5D4037; "            // 咖啡色文字
            "   border: none; "
            "   border-radius: 6px; "
            "   border-bottom: 3px solid #D7CCC8; " // 底部阴影，增加立体感
            "   font-family: 'Microsoft YaHei'; "   // 或者 WenQuanYi
            "   font-size: 22px; "
            "   font-weight: bold; "
            "}"
            "QPushButton:pressed { "
            "   background-color: #FFE0B2; " // 按下变淡橙色
            "   border-bottom: 0px; "        // 阴影消失
            "   margin-top: 3px; "           // 产生下压位移
            "}";

    // 【修改点 3】功能键样式 (Shift, Del, 123)：牛奶咖啡色
    QString funcBtnStyle =
            "QPushButton { "
            "   background-color: #8D6E63; " // 浅咖啡色
            "   color: white; "              // 白色文字
            "   border: none; "
            "   border-radius: 6px; "
            "   border-bottom: 3px solid #5D4037; " // 深色阴影
            "   font-size: 18px; "
            "   font-weight: bold; "
            "}"
            "QPushButton:pressed { "
            "   background-color: #6D4C41; "
            "   border-bottom: 0px; margin-top: 3px; "
            "}";

    // --- 第一行 (q-p) ---
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(6);
    for(int i=0; i<10; i++) {
        QPushButton *btn = new QPushButton();
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setStyleSheet(btnStyle);
        connect(btn, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
        m_letterButtons.append(btn);
        row1->addWidget(btn);
    }
    mainLayout->addLayout(row1);

    // --- 第二行 (a-l) 错位布局 ---
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(6);
    row2->setContentsMargins(30, 0, 30, 0); // 增加缩进，更像真实键盘
    for(int i=0; i<9; i++) {
        QPushButton *btn = new QPushButton();
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setStyleSheet(btnStyle);
        connect(btn, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
        m_letterButtons.append(btn);
        row2->addWidget(btn);
    }
    mainLayout->addLayout(row2);

    // --- 第三行 (Shift, z-m, Del) ---
    QHBoxLayout *row3 = new QHBoxLayout();
    row3->setSpacing(6);

    btnShift = new QPushButton("Shift");
    btnShift->setFixedSize(100, 50);
    btnShift->setStyleSheet(funcBtnStyle);
    connect(btnShift, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    row3->addWidget(btnShift);

    for(int i=0; i<7; i++) {
        QPushButton *btn = new QPushButton();
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setStyleSheet(btnStyle);
        connect(btn, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
        m_letterButtons.append(btn);
        row3->addWidget(btn);
    }

    QPushButton *btnDel = new QPushButton("Del");
    btnDel->setFixedSize(100, 50);
    btnDel->setStyleSheet(funcBtnStyle);
    connect(btnDel, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    row3->addWidget(btnDel);
    mainLayout->addLayout(row3);

    // --- 第四行 (123, Space, Done) ---
    QHBoxLayout *row4 = new QHBoxLayout();
    row4->setSpacing(6);

    btnMode = new QPushButton("123");
    btnMode->setFixedSize(110, 50);
    btnMode->setStyleSheet(funcBtnStyle);
    connect(btnMode, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    row4->addWidget(btnMode);

    QPushButton *btnSpace = new QPushButton("Space");
    btnSpace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btnSpace->setFixedHeight(50);
    // 空格键稍微特殊一点，用纯白
    btnSpace->setStyleSheet(
        "QPushButton { "
        "   background-color: #FFFFFF; "
        "   color: #5D4037; "
        "   border-radius: 6px; "
        "   border-bottom: 3px solid #CFD8DC; "
        "   font-size: 18px; "
        "}"
        "QPushButton:pressed { background-color: #ECEFF1; border-bottom: 0px; margin-top: 3px; }"
    );
    connect(btnSpace, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    row4->addWidget(btnSpace);

    // 【修改点 4】完成键：亮橙色 (品牌主色调)
    QPushButton *btnEnter = new QPushButton("完成");
    btnEnter->setFixedSize(130, 50);
    btnEnter->setStyleSheet(
                "QPushButton { "
                "   background-color: #FF8C00; " // 亮橙色
                "   color: white; "
                "   font-weight: bold; "
                "   border-radius: 6px; "
                "   border-bottom: 3px solid #E65100; " // 深橙色阴影
                "   font-size: 20px; "
                "}"
                "QPushButton:pressed { "
                "   background-color: #F57C00; "
                "   border-bottom: 0px; margin-top: 3px; "
                "}"
                );
    connect(btnEnter, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    row4->addWidget(btnEnter);

    mainLayout->addLayout(row4);

    updateKeyLabels();
}

void SoftKeyboard::updateKeyLabels() {
    int count = qMin(m_letterButtons.size(), CHARS_LOWER.size());

    for(int i=0; i<count; i++) {
        if (isNumber) {
            if (i < CHARS_SYMBOL.size())
                m_letterButtons[i]->setText(CHARS_SYMBOL[i]);
            else
                m_letterButtons[i]->setText("");
        } else {
            m_letterButtons[i]->setText(isCapital ? CHARS_UPPER[i] : CHARS_LOWER[i]);
        }
    }

    if (isNumber) {
        btnMode->setText("ABC");
        btnShift->setEnabled(false);
        // 数字模式下，Shift变灰
        btnShift->setStyleSheet("QPushButton { background-color: #5D4037; color: #8D6E63; border:none; border-radius: 6px;}");
    } else {
        btnMode->setText("123");
        btnShift->setEnabled(true);
        if(isCapital) {
            // Shift 激活：变成亮橙色，提示正在大写模式
            btnShift->setStyleSheet("QPushButton { background-color: #FF8C00; color: white; border-radius: 6px; border-bottom: 3px solid #E65100; font-weight: bold;}");
        } else {
            // Shift 普通：恢复咖啡色
            btnShift->setStyleSheet("QPushButton { background-color: #8D6E63; color: white; border-radius: 6px; border-bottom: 3px solid #5D4037; font-weight: bold;}");
        }
    }
}

void SoftKeyboard::onButtonClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString text = btn->text();
    QWidget *focusObj = QApplication::focusWidget();

    if (text == "&&") text = "&";

    if (btn == btnShift) { isCapital = !isCapital; updateKeyLabels(); return; }
    if (btn == btnMode) { isNumber = !isNumber; updateKeyLabels(); return; }

    if (text == "完成") {
        this->hide();
        if (focusObj) {
            QKeyEvent ev(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QCoreApplication::sendEvent(focusObj, &ev);
        }
        return;
    }

    if (!focusObj) return;

    int key = 0;
    QString commitText = text;

    if (text == "Del") {
        key = Qt::Key_Backspace;
        commitText = "";
    } else if (text == "Space") {
        key = Qt::Key_Space;
        commitText = " ";
    } else if (!text.isEmpty()) {
        key = text.at(0).unicode();
    }

    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, commitText);
    QCoreApplication::sendEvent(focusObj, &press);
}

bool SoftKeyboard::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        QLineEdit *edit = qobject_cast<QLineEdit*>(watched);
        if (edit) {
            this->move(0, 220); // 固定底部
            isNumber = false;
            updateKeyLabels();
            this->show();
            this->raise();
        }
    }
    return QDialog::eventFilter(watched, event);
}
