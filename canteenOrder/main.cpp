#include "login.h"
#include "hardwarecontrol.h"
#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QApplication a(argc, argv);
    HardwareControl::instance()->initHardware();
    QPixmap pixmap(":/res/startup.png");
    if (pixmap.isNull()) {
        qDebug() << "warning:picture path can not find....";
        pixmap = QPixmap(800, 480);
        pixmap.fill(QColor("#FF8C00"));
    } else {
        pixmap = pixmap.scaled(800, 480, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    QSplashScreen splash(pixmap);
    splash.setFixedSize(800, 480);
    splash.show(); // 显示画面

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, SLOT(quit()));
    loop.exec();
    QString qss =
            "QWidget { background-color: #FFF5E6; color: #5D4037; font-family: 'Microsoft YaHei'; font-size: 18px; }"
            "QLineEdit { border: 2px solid #D7CCC8; border-radius: 8px; padding: 10px; background-color: white; font-size: 20px; }"
            "QLineEdit:focus { border: 2px solid #FF8C00; background-color: #FFFDE7; }"
            "QPushButton { background-color: #FF8C00; color: white; border-radius: 10px; padding: 10px; font-weight: bold; font-size: 20px; border-bottom: 4px solid #E65100; }"
            "QPushButton:pressed { background-color: #E65100; border-bottom: 0px; margin-top: 4px; }"
            "QPushButton#SecondaryButton { background-color: #A1887F; border-bottom: 4px solid #5D4037; }"
            "QPushButton#SecondaryButton:pressed { background-color: #8D6E63; border-bottom: 0px; margin-top: 4px; }"
            "QLabel#Title { font-size: 32px; font-weight: bold; color: #BF360C; }"
            "SoftKeyboard { background-color: #3E2723; border-top: 4px solid #5D4037; border-top-left-radius: 20px; border-top-right-radius: 20px; }"
            "SoftKeyboard QPushButton { background-color: #FFF8E1; color: #3E2723; border: none; border-radius: 6px; font-size: 20px; font-weight: bold; border-bottom: 3px solid #D7CCC8; }"
            "SoftKeyboard QPushButton:pressed { background-color: #FFECB3; border-bottom: 0px; margin-top: 3px; }"
            "SoftKeyboard QPushButton#SpaceKey { background-color: white; border-bottom: 3px solid #CFD8DC; }"
            "SoftKeyboard QPushButton#FuncKey { background-color: #6D4C41; color: white; border-bottom: 3px solid #3E2723; }"
            "SoftKeyboard QPushButton#FuncKey:pressed { background-color: #5D4037; border-bottom: 0px; margin-top: 3px; }"
            "SoftKeyboard QPushButton#ActionKey { background-color: #FF6F00; color: white; border-bottom: 3px solid #BF360C; }"
            "SoftKeyboard QPushButton#ActionKey:pressed { background-color: #E65100; border-bottom: 0px; margin-top: 3px; }";
    a.setStyleSheet(qss);
    login w;
    splash.finish(&w);
    w.show();

    return a.exec();
}
