#ifndef TCPSERVER_H
#define TCPSERVER_H

// qt
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDomDocument>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <vector>
#include <QMessageBox>

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer();

    void startudp();
    QByteArray create_xml(int cmd, float x, float y, float z, float a, float b, float c);

signals:
    void newthreadstartsignals();
    void showMessage(const QString &title, const QString &message);  // 新增：显示消息信号

public slots:
    void newthreadstartslots();
    void acceptConnection();
    QTcpSocket* getClientConnection() const;

private:
    QTcpServer* localtdp_server;
    QTcpSocket* clientConnection;
    QHostAddress m_IP_local;
    quint16 m_Port_Rx;
};

struct Poseone
{
    int cmd;
    float x;
    float y;
    float z;
    float a;
    float b;
    float c;
};

extern Poseone posetemp;

#endif // TCPSERVER_H
