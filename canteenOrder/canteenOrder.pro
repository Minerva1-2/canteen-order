QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = canteenOrder
TEMPLATE = app

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dbmanager.cpp \
    hardwarecontrol.cpp \
    haveordered.cpp \
    login.cpp \
    main.cpp \
    maininterface.cpp \
    mainwindow.cpp \
    minimqtt.cpp \
    orderwidget.cpp \
    paywidget.cpp \
    register.cpp \
    settlewidget.cpp \
    softkeyboard.cpp \
    videowidget.cpp

HEADERS += \
    dbmanager.h \
    hardwarecontrol.h \
    haveordered.h \
    login.h \
    maininterface.h \
    mainwindow.h \
    minimqtt.h \
    orderwidget.h \
    paywidget.h \
    register.h \
    settlewidget.h \
    softkeyboard.h \
    videowidget.h

FORMS += \
    mainwindow.ui

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

/**
    MQTT_IP:地址
    MQTT_PORT：端口号
*/
DEFINES += MQTT_IP=\\\"47.108.190.17\\\" \
            MQTT_PORT=1883
