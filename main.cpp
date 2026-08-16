#include "mainwindow1.h"
#include "Phaselink/mainwindow.h"
#include "mainwindow2.h"
#include <windows.h>
#include <algorithm>
#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QPushButton>
#include <QDebug>
#include <QTemporaryFile>
#include <QTimer>
#include <QTranslator>
#include <QSurfaceFormat>
#include <QSplashScreen>
#include <QString>
#include <QTextStream>
#include <QtMath>
#include <vtkObject.h>
#include <vtkOutputWindow.h>

#include "simulation/SimDataPlayer.h"

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

    QCommandLineParser parser;
    parser.setApplicationDescription("SoundScan");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("simulate-test",
                                        "Run a short built-in simulation and exit when it finishes."));
    parser.process(a);

    // 禁止 VTK 弹出独立输出窗口（错误/警告信息不再弹窗）
    vtkObject::GlobalWarningDisplayOff();
    vtkOutputWindow::GetInstance()->SetDisplayModeToNever();

    QPixmap pixmap(":/images/images/start.png");
    QSplashScreen splash(pixmap);
    splash.show();

    QTranslator lang;
    bool loaded = lang.load(":/phaselink_CN.qm");
    qDebug() << "Translation loaded:" << loaded;  // 必须输出 true
    qApp->installTranslator(&lang);

    // 创建三个窗口实例
    MainWindow1 w;    // 参数配置
    MainWindow w3;     // 调试扫描
    MainWindow2 w2;    // 扫描显示
    w2.setMainWindow(&w);
    w3.setWindowFlags(Qt::FramelessWindowHint);

    // 设置窗口标题
    w.setWindowTitle("参数配置");
    w3.setWindowTitle("调试扫描");
    w2.setWindowTitle("扫描显示");

    // 默认显示主窗口全屏
    w2.showFullScreen();
    w.showFullScreen();
    splash.finish(&w2);

    // 参数配置->调试扫描
    QPushButton *btnToW3 = w.findChild<QPushButton*>("pushButton_45");  // 调试按钮
    QObject::connect(btnToW3, &QPushButton::clicked, [&]() {
        emit App::getInstance()->refresh_Allpara();
        if (w3.isVisible()) {
            w3.raise();
            w3.activateWindow();
        } else {
            w3.showFullScreen();
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
    QPushButton *btnToW2 = w.findChild<QPushButton*>("pushButton_33");  // 扫描显示按钮
    QObject::connect(btnToW2, &QPushButton::clicked, [&]() {
        w2.raise();
        w2.activateWindow();
    });

    // 扫查显示->参数配置
    QPushButton *btnBackFromW2 = w2.findChild<QPushButton*>("pushButton_33");  // 返回按钮
    QObject::connect(btnBackFromW2, &QPushButton::clicked, [&]() {
        w.raise();
        w.activateWindow();
    });

    // ===== 模拟数据源（点击“扫描开始”时启动）=====
    // 环境变量 SIM_CSV 指定录制文件（分号分隔）；未设置时使用默认 Downloads 录制文件。
    // 存在录制文件时：注册给主界面，点击“扫描开始”后才开始回放 + 3D 开始绘制，
    // 整条链路用模拟数据运行。--simulate-test 模式除外：启动后 3 秒自动开始回放并退出。
    {
        QStringList simFiles;
        QTemporaryFile *testCsv = nullptr;
        const bool simulateTest = parser.isSet("simulate-test");
        QByteArray simEnv = qgetenv("SIM_CSV");
        if (simulateTest) {
            testCsv = new QTemporaryFile(&a);
            testCsv->setAutoRemove(true);
            if (testCsv->open()) {
                QTextStream out(testCsv);
                out << "X,Y,Z,A,B,C,SI";
                for (int i = 1; i <= 49; ++i)
                    out << ",AMP_" << i << ",TOF_" << i;
                out << ",BEAM,LX,LY\n";
                for (int row = 0; row < 250; ++row) {
                    out << row * 0.1 << ',' << row * 0.05 << ',' << 10.0 + row * 0.01
                        << ",0,0,0," << 0.5 + 0.1 * qSin(row / 20.0);
                    for (int i = 0; i < 49; ++i)
                        out << ',' << (0.2 + 0.001 * row + i * 0.002)
                            << ',' << (1.0 + i * 0.01);
                    out << ",49," << row * 0.1 << ',' << row * 0.05 << '\n';
                }
                out.flush();
                testCsv->flush();
                testCsv->close();
                simFiles << testCsv->fileName();
                qDebug() << "[AutoSim] using built-in test data:" << testCsv->fileName();
            }
        } else if (!simEnv.isEmpty()) {
            simFiles = QString::fromLocal8Bit(simEnv).split(';', Qt::SkipEmptyParts);
        } else {
            const QString def1 = "C:/Users/23714/Downloads/scan_20260717_141034.csv";
            const QString def2 = "C:/Users/23714/Downloads/scan_20260717_133933.csv";
            if (QFile::exists(def1))
                simFiles << def1;
            if (QFile::exists(def2))
                simFiles << def2;
        }

        if (!simFiles.isEmpty()) {
            SimDataPlayer *sim = new SimDataPlayer(&a);
            if (sim->loadCsv(simFiles)) {
                // 交由主界面在点击“扫描开始”时启动模拟数据源
                w.setSimulationPlayer(sim);
                QObject::connect(sim, &SimDataPlayer::finished, &a, [&]() {
                    qDebug() << "[AutoSim] simulation finished; stopping drawing";
                    if (w2.getMainWindow3())
                        w2.getMainWindow3()->finishDrawing();
                    if (simulateTest)
                        QTimer::singleShot(200, &a, &QCoreApplication::quit);
                });
                if (simulateTest) {
                    QTimer::singleShot(3000, &a, [&]() {
                        qDebug() << "[AutoSim] starting simulation and drawing";
                        sim->start();
                        if (w2.getMainWindow3())
                            w2.getMainWindow3()->startDrawing();
                    });
                } else {
                    qDebug() << "[AutoSim] simulation ready; will start on scan-start click";
                }
            }
        } else {
            qDebug() << "[AutoSim] no recording file found; simulation skipped";
        }
    }

    return a.exec();
}
