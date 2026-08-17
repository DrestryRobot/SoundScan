#ifndef MAINWINDOW2_H
#define MAINWINDOW2_H

#include <client.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QTranslator>
#include "Phaselink/datadispatch.h"
#include "mainwindow1.h"
#include "ui_mainwindow2.h"
#include "Phaselink/dialog/viewmodel.h"
#include "Phaselink/dialog/viewwidget.h"
#include <mainwindow3.h>

namespace Ui {
class MainWindow2;
}

class MainWindow2 : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow2(QWidget *parent = nullptr);

    ~MainWindow2();

    static MainWindow2 *s_instance;

    void setMainWindow(MainWindow1 *mainWin) { mainWindow = mainWin; }
    MainWindow3 *getMainWindow3() const { return mainWindow3; }

    void onDataPacket(const QByteArray &data, int deviceId);
private:

    void initWidget(); // 初始化超声界面

    void initSlot(); // 初始化超声信号

signals:
    void requestRegisterDebugOutput(QLineEdit* widget);

private slots:

    void on_pushButton_29_clicked(); // 扫描开始（快捷指令）

    void on_pushButton_30_clicked(); // 扫描暂停（快捷指令）

    void on_pushButton_38_clicked(); // 扫描结束（快捷指令）

    void on_pushButton_34_clicked(); // 退出系统（快捷指令）

    void slot_rulerWidgetChanged();

    void slot_rulerProbeChanged();

private:
    Ui::MainWindow2 *ui;

    MainWindow1 *mainWindow = nullptr;

    Client &config;
    bool IsScaning = false;
    ConfigState IsConnect = ConfigState::Unconnected;
    QTranslator lang;
    QVector<int> AmpData; // 保存C扫的测量值幅值数据
    std::string ipAddress;
    QByteArray curDatapacket;
    PacketDataSaver saver;
    QMap<quint32, QByteArray> m_packet;
    DataProcessor *m_dataProcessor = nullptr;
    QThread *m_processorThread = nullptr;
    QThread* m_3dThread;
    ViewModel *m_3dView = nullptr;
    int m_beamCount = 49;
    QObject *m_3dWorker = nullptr;
    DataDispatch::SinkPtr m_directSink;

    QDate today = QDate::currentDate();
    QString dateStr = today.toString("yyyy-MM-dd");
    QString logPath = "C:/超声扫描/日志/系统日志" + dateStr + ".txt";

    MainWindow3 *mainWindow3 = nullptr;

    void setupChildWindow();
};

#endif // MAINWINDOW2_H
