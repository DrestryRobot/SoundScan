// UltrasoundReceiver.h
#ifndef ULTRASOUNDRECEIVER_H
#define ULTRASOUNDRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include <QList>  // 添加

class UltrasoundReceiver : public QObject
{
    Q_OBJECT

public:
    explicit UltrasoundReceiver(QObject *parent = nullptr);
    ~UltrasoundReceiver();

    void start(quint16 port);
    void stop();

signals:
    // 改为 QList
    void dataReady(int64_t frameId, const QList<float>& amps, const QList<float>& tofs);
    void imageReady(int beamCount, int pointQuantity, const QVector<uint8_t>& imageData);

private slots:
    void readDatagram();

private:
    void parseData(const QByteArray& data);

    QUdpSocket* m_socket;
};

#endif // ULTRASOUNDRECEIVER_H