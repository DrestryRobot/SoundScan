// UltrasoundReceiver.cpp
#include "UltrasoundReceiver.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <cmath>

UltrasoundReceiver::UltrasoundReceiver(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &UltrasoundReceiver::readDatagram);
}

UltrasoundReceiver::~UltrasoundReceiver()
{
    stop();
}

void UltrasoundReceiver::start(quint16 port)
{
    if (m_socket->bind(port)) {
        qDebug() << "超声接收器启动成功，端口:" << port;
    } else {
        qDebug() << "超声接收器启动失败:" << m_socket->errorString();
    }
}

void UltrasoundReceiver::stop()
{
    if (m_socket) {
        m_socket->close();
    }
}

void UltrasoundReceiver::readDatagram()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        parseData(datagram.data());
    }
}

// UltrasoundReceiver.cpp - 修改 parseData 函数
void UltrasoundReceiver::parseData(const QByteArray& data)
{
    const int beamCount = 49;
    const int pointQuantity = 1024;

    int expectedSize = beamCount * (pointQuantity + 16) * sizeof(int16_t);

    // 添加调试：打印接收到的数据大小
    static int packetCount = 0;
    packetCount++;
    if (packetCount % 100 == 0) {
        qDebug() << "收到数据包大小:" << data.size() << "期望大小:" << expectedSize;
    }

    if (data.size() < expectedSize) {
        qDebug() << "数据包太小，跳过:" << data.size() << "<" << expectedSize;
        return;
    }

    QVector<uint8_t> imageData;
    imageData.reserve(beamCount * pointQuantity);

    QList<float> amps;
    QList<float> tofs;
    amps.reserve(49);
    tofs.reserve(49);

    const int16_t* rawData = reinterpret_cast<const int16_t*>(data.constData());
    double maxAmp = 32768.0;

    for (int i = 0; i < beamCount; i++) {
        int beamOffset = i * (pointQuantity + 16);
        int measureOffset = beamOffset + pointQuantity;

        float amp = 0.0f;
        float tof = 0.0f;

        if (measureOffset + 7 <= data.size() / sizeof(int16_t)) {
            int ampIndex = measureOffset + 4;
            if (ampIndex < data.size() / sizeof(int16_t)) {
                amp = std::abs(rawData[ampIndex]) / maxAmp;
                amp = qBound(0.0f, amp, 1.0f);
            }

            int tofIndex = measureOffset;
            if (tofIndex < data.size() / sizeof(int16_t)) {
                tof = rawData[tofIndex] / (float)pointQuantity;
                tof = qBound(0.0f, tof, 1.0f);
            }
        }
        amps.append(amp);
        tofs.append(tof);

        for (int j = 0; j < pointQuantity; j++) {
            int idx = beamOffset + j;
            if (idx < data.size() / sizeof(int16_t)) {
                double pct = std::abs(rawData[idx]) * 100.0 / maxAmp;
                int color = qBound(0, (int)(pct * 2.55), 255);
                imageData.append((uint8_t)color);
            } else {
                imageData.append(0);
            }
        }
    }

    static int64_t frameId = 0;

    // 添加调试：打印幅值数据
    if (frameId % 100 == 0) {
        qDebug() << "超声帧" << frameId << "中心幅值:" << amps[24] << "图像数据大小:" << imageData.size();
    }

    emit dataReady(frameId++, amps, tofs);
    emit imageReady(beamCount, pointQuantity, imageData);
}