#include "ads_poller.h"

#include "ads_client.h"
#include "3dscan/scandata.h"

#include <QDebug>

// adsClient 全局实例定义在 mainwindow1.cpp
extern ads_client adsClient;

AdsStatusPoller::AdsStatusPoller(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &AdsStatusPoller::poll);
}

void AdsStatusPoller::start()
{
    poll(); // 先立即采样一次，避免界面等待 100ms
    m_timer.start();
}

void AdsStatusPoller::stop()
{
    m_timer.stop();
}

void AdsStatusPoller::poll()
{
    Status s;

    // 机器人位姿（由 UDP 线程/Delmia 线程写入的全局量）
    s.robotX = robot_x;
    s.robotY = robot_y;
    s.robotZ = robot_z;
    s.robotA = robot_a;
    s.robotB = robot_b;
    s.robotC = robot_c;

    // 龙门使能/位置/速度（阻塞式 ADS 读，放在后台线程）
    s.xEnable = adsClient.getIntVal(0x67CE0) != 0;
    s.yEnable = adsClient.getIntVal(0x67D60) != 0;
    s.xPos = adsClient.getFloatVal(0x67D18);
    s.xVel = adsClient.getFloatVal(0x67D1C);
    s.yPos = adsClient.getFloatVal(0x67D98);
    s.yVel = adsClient.getFloatVal(0x67D9C);

    // 龙门运动标志
    s.xMove1 = adsClient.getBoolVal(0x5EBDE);
    s.xMove2 = adsClient.getBoolVal(0x5EBDD);
    s.xMove3 = adsClient.getBoolVal(0x5EBDB);
    s.xMove4 = adsClient.getBoolVal(0x5EBDC);
    s.yMove1 = adsClient.getBoolVal(0x5EC2E);
    s.yMove2 = adsClient.getBoolVal(0x5EC2D);
    s.yMove3 = adsClient.getBoolVal(0x5EC2B);
    s.yMove4 = adsClient.getBoolVal(0x5EC2C);

    // 当前程序号
    s.progNo = adsClient.getIntVal(0x5EB08);

    // 程序号确认位：置位后写回 0（与原有逻辑一致）
    if (adsClient.getIntVal(1))
        adsClient.setIntVal(1, 0);

    // 机器人模式
    s.modeT1 = adsClient.getIntVal(0x5EB0F) != 0;
    s.modeT2 = adsClient.getIntVal(0x5EB90) != 0;
    s.modeAUT = adsClient.getIntVal(0x5EB91) != 0;
    s.modeEXT = adsClient.getIntVal(0x5EB92) != 0;

    // 龙门位置（同步到全局量，供 3D 使用）
    s.longmenX = adsClient.getFloatVal(0x67D18);
    s.longmenY = adsClient.getFloatVal(0x67D98);
    {
        QMutexLocker locker(&g_scanDataMutex);
        longmen[0] = s.longmenX;
        longmen[1] = s.longmenY;
    }

    // 临时诊断：每 2 秒输出一次，确认轮询线程在跑
    static int diagCount = 0;
    if (++diagCount % 20 == 1)
        qDebug() << "[AdsPoll] poll ok, xPos=" << s.xPos << " yPos=" << s.yPos
                 << " progNo=" << s.progNo;

    emit statusReady(s);
}
