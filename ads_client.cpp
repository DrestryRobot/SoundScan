#include "ads_client.h"

using namespace std;

RoboMotionInfo g_RoboMotionInfo;
ads_client::ads_client() {}

void ads_client::AdsConnectLocal()
{
    QMutexLocker locker(&m_mutex);
    pAddr = &Addr;

    nPort = AdsPortOpen();
    if (nPort == 0) {
        m_connected = false;
        qDebug() << "ADS local port open failed";
        return;
    }
    nErr = AdsGetLocalAddress(pAddr);
    if (nErr)
        qDebug() << "错误：获取本地ADS地址失败：" << nErr;
    else
        qDebug() << "ADS连接成功！";

    pAddr->port = 851; // 端口设置在打开之后

    nErr = AdsSyncReadStateReq(pAddr, &nAdsState, &nDeviceState);
    m_connected = (nErr == 0);
    if (nErr)
        qDebug() << "错误：读取PLC状态失败：" << nErr;
    else {
        if (nAdsState == ADSSTATE_RUN)
            // qDebug() << "PLC程序正在运行中！";
            qDebug() << "PLC系统运行正常";
        else if (nAdsState == ADSSTATE_STOP) {
            // qDebug() << "PLC程序已停止！";
            qDebug() << "PLC系统运行错误或未启动";
            qDebug() << "正在尝试启动程序...";
            nAdsState = ADSSTATE_RUN;
            nErr = AdsSyncWriteControlReq(pAddr, nAdsState, nDeviceState, 0, NULL);
            if (nErr)
                qDebug() << "错误：启动PLC程序失败：" << nErr;
            else
                qDebug() << "PLC程序已成功启动！";
        }
    }
}

void ads_client::AdsConnectRemote()
{
    QMutexLocker locker(&m_mutex);
    Addr = {{192,168,10,22,1,1}}; // 定义AMS地址变量
    pAddr = &Addr;
    nPort = AdsPortOpen();
    if (nPort == 0) {
        m_connected = false;
        qDebug() << "ADS port open failed (TwinCAT not installed?)";
        return;
    }
    pAddr->port = 851;

    nErr = AdsSyncReadStateReq(pAddr, &nAdsState, &nDeviceState);
    m_connected = (nErr == 0);
    if (nErr)
        // qDebug() << "错误：读取PLC状态失败：" << nErr;
        qDebug() << "PLC系统运行错误或未启动";
    else {
        if (nAdsState == ADSSTATE_RUN)
            // qDebug() << "PLC程序正在运行中！";
            qDebug() << "PLC系统运行正常";
        else if (nAdsState == ADSSTATE_STOP) {
            // qDebug() << "PLC程序已停止！";
            qDebug() << "PLC系统运行错误或未启动";
            qDebug() << "正在尝试启动程序...";
            nAdsState = ADSSTATE_RUN;
            nErr = AdsSyncWriteControlReq(pAddr, nAdsState, nDeviceState, 0, NULL);
            if (nErr)
                qDebug() << "错误：启动PLC程序失败：" << nErr;
            else
                qDebug() << "PLC程序已成功启动！";
        }
    }
}

void ads_client::getRoboMotionInfo()
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return;
    AdsSyncReadReq(pAddr, 0x4020, 100, sizeof(g_RoboMotionInfo), &g_RoboMotionInfo);
}

float ads_client::getFloatVal(int offsetAddr)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return 0.0f;
    float getVal = 0.0f;

    AdsSyncReadReq(pAddr, 0x4020, offsetAddr, sizeof(float), &getVal);

    return getVal;
}

void ads_client::setFloatVal(int offsetAddr, float setVal)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return;
    AdsSyncWriteReq(pAddr, 0x4020, offsetAddr, sizeof(float), &setVal);
}

short ads_client::getIntVal(int offsetAddr)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return 0;
    short getVal = 0;

    AdsSyncReadReq(pAddr, 0x4020, offsetAddr, sizeof(short), &getVal);

    return getVal;
}

bool ads_client::getBoolVal(int offsetAddr)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return false;
    bool getVal = false;

    AdsSyncReadReq(pAddr, 0x4020, offsetAddr, sizeof(bool), &getVal);

    return getVal;
}

void ads_client::setIntVal(int offsetAddr, short setVal)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return;
    AdsSyncWriteReq(pAddr, 0x4020, offsetAddr, sizeof(short), &setVal);
}

