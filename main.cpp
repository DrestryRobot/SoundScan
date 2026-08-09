#include "mainwindow.h"
#include "mainwindow3.h"
#include "mainwindow5.h"
#include <windows.h>
#include <algorithm>
#include <QApplication>
#include <QByteArray>
#include <QPushButton>
#include <QDebug>
#include <QTranslator>
#include <QSurfaceFormat>
#include <QSplashScreen>
#include <QString>
#include <vtkObject.h>
#include <vtkOutputWindow.h>

int main(int argc, char *argv[])
{
    // 自适应不同分辨率：界面按 2560x1440 设计，这里根据主屏物理尺寸和系统缩放
    // 自动计算 Qt 缩放因子，保证 2K/4K 屏幕都能完整显示，无需修改系统设置。
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const int designW = 2560;
        const int designH = 1440;
        const int physW = GetSystemMetrics(SM_CXSCREEN);
        const int physH = GetSystemMetrics(SM_CYSCREEN);
        const UINT dpi = GetDpiForSystem();
        const double nativeDpr = dpi / 96.0;
        double scale = std::min(physW / (double)designW, physH / (double)designH) / nativeDpr;
        if (scale > 0.1 && scale < 10.0)
            qputenv("QT_SCALE_FACTOR", QByteArray::number(scale, 'g', 8));
    }

    QApplication a(argc, argv);

    // 禁止 VTK 弹出独立输出窗口（错误/警告信息不再弹窗）
    vtkObject::GlobalWarningDisplayOff();
    vtkOutputWindow::GetInstance()->SetDisplayModeToNever();

    QPixmap pixmap(":/images/images/splash1.png");
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
    splash.finish(&w5);

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
        w5.raise();
        w5.activateWindow();
    });

    // 扫查显示->参数配置
    QPushButton *btnBackFromW5 = w5.findChild<QPushButton*>("pushButton_33");  // 返回按钮
    QObject::connect(btnBackFromW5, &QPushButton::clicked, [&]() {
        w.raise();
        w.activateWindow();
    });

    return a.exec();
}
