#include "mainwindow.h"
#include "mainwindow3.h"
#include "mainwindow5.h"
#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QTranslator>
#include <QSurfaceFormat>
#include <QSplashScreen>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/images/images/圆角-层压结构高速成像检测系统.png");
    QSplashScreen splash(pixmap);
    splash.show();

    QTranslator lang;
    bool loaded = lang.load(":/phaselink_CN.qm");
    qDebug() << "Translation loaded:" << loaded;  // 必须输出 true
    qApp->installTranslator(&lang);

    // 创建三个窗口实例
    MainWindow w;      // 参数配置
    MainWindow3 w3;    // 调试扫描
    MainWindow5 w5;    // 扫描显示
    w5.setMainWindow(&w);
    w3.setWindowFlags(Qt::FramelessWindowHint);

    // 设置窗口标题
    w.setWindowTitle("参数配置");
    w3.setWindowTitle("调试扫描");
    w5.setWindowTitle("扫描显示");

    // 默认显示主窗口全屏
    w5.showFullScreen();
    w.showFullScreen();

    // 参数配置->调试扫描
    QPushButton *btnToW3 = w.findChild<QPushButton*>("pushButton_45");  // 调试按钮
    QObject::connect(btnToW3, &QPushButton::clicked, [&]() {
        emit App::getInstance()->refresh_Allpara();
        if (w3.isVisible()) {
            w3.raise();
            w3.activateWindow();
        } else {
            w3.show();
        }
    });

    // 调试扫描->参数配置
    QPushButton *btnBackFromW3 = w3.findChild<QPushButton*>("pushButton");  // 返回按钮
    QObject::connect(btnBackFromW3, &QPushButton::clicked, [&]() {
        emit App::getInstance()->refresh_Allpara();
        w.showFullScreen();
        w3.close();
    });

    // 参数配置->扫查显示
    QPushButton *btnToW5 = w.findChild<QPushButton*>("pushButton_33");  // 扫描显示按钮
    QObject::connect(btnToW5, &QPushButton::clicked, [&]() {
        w5.showFullScreen();
    });

    // 扫查显示->参数配置
    QPushButton *btnBackFromW5 = w5.findChild<QPushButton*>("pushButton_33");  // 返回按钮
    QObject::connect(btnBackFromW5, &QPushButton::clicked, [&]() {
        w.showFullScreen();
    });

    return a.exec();
}