#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QXmlStreamReader>
#include "dialog/viewmodel.h"
#include <QVector>

// udpserver.h 中添加
class UltrasoundWindow;  // 前向声明

class UdpServer : public QObject
{
    Q_OBJECT
public:
    explicit UdpServer(QObject *parent = nullptr);
    void startListening(quint16 port = 59153, const QString &bindAddress = "10.168.1.200");

public slots:
    void stop();

signals:
    void newRobotData(double x, double y, double z, double a, double b, double c);  // RIst信号
    void newJointData(double a1, double a2, double a3, double a4, double a5, double a6); // AIPos信号
    void ipocUpdated(quint32 ipoc); // IPOC计数器信号

    // // 添加信号 - 只需要这一个函数！
    // void robotDataReady(quint32 ipoc, float x, float y, float z,
    //                     float a, float b, float c);


private slots:
    void readPendingDatagrams();

private:
    QUdpSocket *m_udpSocket;
    void parseXmlData(const QByteArray &xmlData);
};

#endif // UDPSERVER_H
