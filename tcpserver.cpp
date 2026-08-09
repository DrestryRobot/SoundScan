#include "tcpserver.h"

Poseone posetemp;

bool tcp_ok = false;

TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    localtdp_server = nullptr;
    m_IP_local = QHostAddress("10.168.1.200"); // 可根据需要修改为具体 IP
    m_Port_Rx = 59152;                     // 可根据需要修改为具体端口
}

TcpServer::~TcpServer()
{
    if (localtdp_server) {
        localtdp_server->close();
        delete localtdp_server;
    }
}

void TcpServer::startudp()
{
    QThread::msleep(500);

    connect(this, &TcpServer::newthreadstartsignals, this, &TcpServer::newthreadstartslots);
    emit newthreadstartsignals();
}

void TcpServer::newthreadstartslots()
{
    localtdp_server = new QTcpServer(this);
    if (!localtdp_server->listen(m_IP_local, m_Port_Rx)) {
        // qDebug() << "TCP服务器监听失败：" << m_IP_local.toString() << ":" << m_Port_Rx;
        qDebug() << "上位机与PLC系统 TCP 通讯失败，无机器人关节角数据，无法记录点位";
        return;
    }
    // qDebug() << "TCP服务器正在监听：" << m_IP_local.toString() << ":" << m_Port_Rx;
    qDebug() << "上位机与PLC系统 TCP 通讯成功";
    connect(localtdp_server, &QTcpServer::newConnection, this, &TcpServer::acceptConnection, Qt::DirectConnection);
}

void TcpServer::acceptConnection()
{
    clientConnection = localtdp_server->nextPendingConnection();
    if (clientConnection) {
        // qDebug() << "已接受来自以下地址的连接：" << clientConnection->peerAddress().toString();
        qDebug() << "KUKA 已连接，请记录位姿点";
        // 发送信号到主线程显示弹窗
        emit showMessage(tr("提示"), tr("KUKA 已连接，请记录位姿点"));
        tcp_ok = true;
    } else {
        qDebug() << "无待处理的连接。";
    }
}

QByteArray TcpServer::create_xml(int cmd, float x, float y, float z, float a, float b, float c)
{
    QFile file("temp.xml");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "打开文件进行写入失败。";
        return QByteArray();
    }

    QDomDocument document;
    QDomElement root_elem = document.createElement("Robots");
    document.appendChild(root_elem);

    QDomElement item1 = document.createElement("Command");
    item1.appendChild(document.createTextNode(QString::number(cmd)));
    root_elem.appendChild(item1);

    QDomElement item2 = document.createElement("Pos");
    item2.setAttribute("X", QString::number(x));
    item2.setAttribute("Y", QString::number(y));
    item2.setAttribute("Z", QString::number(z));
    item2.setAttribute("A", QString::number(a));
    item2.setAttribute("B", QString::number(b));
    item2.setAttribute("C", QString::number(c));
    root_elem.appendChild(item2);

    QTextStream out(&file);
    document.save(out, 4);
    file.close();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "打开文件进行读取失败。";
        return QByteArray();
    }

    QTextStream in(&file);
    QByteArray Data_Xml_Tx = in.readAll().toLocal8Bit();
    file.close();

    return Data_Xml_Tx;
}

QTcpSocket* TcpServer::getClientConnection() const {
    return clientConnection;
}
