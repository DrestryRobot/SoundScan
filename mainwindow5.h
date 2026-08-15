#ifndef MAINWINDOW5_H
#define MAINWINDOW5_H

#include <client.h>
#include <QMainWindow>
#include <QDockWidget>
#include "datadispatch.h"
#include <configwindow.h>
#include "ui_mainwindow5.h"
#include "dialog/viewmodel.h"
#include "dialog/viewwidget.h"

#include <mainwindow7.h>

namespace Ui {
class MainWindow5;
}

class MainWindow5 : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow5(QWidget *parent = nullptr);

    ~MainWindow5();

    static MainWindow5 *s_instance;

    void setMainWindow(ConfigWindow *mainWin) { mainWindow = mainWin; }
    MainWindow7 *getMainWindow7() const { return mainWindow7; }

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
    Ui::MainWindow5 *ui;

    ConfigWindow *mainWindow = nullptr;

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

    MainWindow7 *mainWindow7 = nullptr;

    void setupChildWindow();
};

#endif // MAINWINDOW5_H
