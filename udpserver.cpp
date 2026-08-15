#include "udpserver.h"
#include <QNetworkDatagram>
#include <QDebug>

#include <QDateTime>
#include "3DScan/scandata.h"

UdpServer::UdpServer(QObject *parent) : QObject(parent)
{
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpServer::readPendingDatagrams);
}

void UdpServer::stop()
{
    if (m_udpSocket) {
        m_udpSocket->close();
    }
}

void UdpServer::startListening(quint16 port, const QString &bindAddress)
{
    QThread::msleep(1000);

    if (m_udpSocket->bind(QHostAddress(bindAddress), port)) {
        // qDebug() << "UDP服务器正在监听：" << bindAddress << ":" << port;
        qDebug() << "上位机与PLC系统 RSI 通讯成功";
    } else {
        // qWarning() << "UDP套接字绑定失败：" << m_udpSocket->errorString();
        qDebug() << "上位机与PLC系统 RSI 通讯失败，无机器人关节角数据，无法成像";
    }
}

void UdpServer::readPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        QByteArray xmlData = datagram.data();
        parseXmlData(xmlData);
    }
}

void UdpServer::parseXmlData(const QByteArray &xmlData)
{
    QXmlStreamReader xml(xmlData);
    static double a1,a2,a3,a4,a5,a6;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "RIst") {
            // 解析笛卡尔坐标 (X/Y/Z/A/B/C)
            robot_x = xml.attributes().value("X").toDouble();
            robot_y = xml.attributes().value("Y").toDouble();
            robot_z = xml.attributes().value("Z").toDouble();
            robot_a = xml.attributes().value("A").toDouble();
            robot_b = xml.attributes().value("B").toDouble();
            robot_c = xml.attributes().value("C").toDouble();
            //emit newRobotData(x, y, z, a, b, c);
        }
        else if (xml.isStartElement() && xml.name().toString() == "AIPos") {
            // 解析关节角 (A1-A6)
            a1 = xml.attributes().value("A1").toDouble();
            a2 = xml.attributes().value("A2").toDouble();
            a3 = xml.attributes().value("A3").toDouble();
            a4 = xml.attributes().value("A4").toDouble();
            a5 = xml.attributes().value("A5").toDouble();
            a6 = xml.attributes().value("A6").toDouble();
            //emit newJointData(a1, a2, a3, a4, a5, a6);
        }
        else if (xml.isStartElement() && xml.name().toString() == "IPOC") {
            // 解析IPOC计数器
            quint32 ipoc = xml.readElementText().toUInt();
            emit ipocUpdated(ipoc);

            // qDebug() << QString("AIPos A1=%2 A2=%3 A3=%4 A4=%5 A5=%6 A6=%7 [IPOC=%1]")
            //                 .arg(ipoc)
            //                 .arg(a1, 0, 'f', 4)
            //                 .arg(a2, 0, 'f', 4)
            //                 .arg(a3, 0, 'f', 4)
            //                 .arg(a4, 0, 'f', 4)
            //                 .arg(a5, 0, 'f', 4)
            //                 .arg(a6, 0, 'f', 4);

            // qDebug() << QString("RIst X=%2 Y=%3 Z=%4 A=%5 B=%6 C=%7 [IPOC=%1]")
            //                 .arg(ipoc)
            //                 .arg(robot_x, 0, 'f', 4)
            //                 .arg(robot_y, 0, 'f', 4)
            //                 .arg(robot_z, 0, 'f', 4)
            //                 .arg(robot_a, 0, 'f', 4)
            //                 .arg(robot_b, 0, 'f', 4)
            //                 .arg(robot_c, 0, 'f', 4);

            robot_ipoc = ipoc;

            m_libKuka3D = Kuka3D::LibKuka3D::getInstance();

            m_libKuka3D->addRobotPosition(robot_x, robot_y, robot_z, robot_a, robot_b, robot_c, 0, 0, robot_ipoc);

            QString responseXml = R"(<Sen Type="ImFree"><RKorr A1="0.0000" A2="0.0000" A3="0.0000" A4="0.0000" A5="0.0000" A6="0.0000" /><IPOC>%1</IPOC></Sen>)";

            QByteArray sendData = responseXml.arg(ipoc).toUtf8();

            m_udpSocket->writeDatagram(sendData, QHostAddress("10.168.1.50"), 53453);// 53453
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML parse error:" << xml.errorString();
    }
}
