#ifndef ADS_CLIENT_H
#define ADS_CLIENT_H

// qt
#include <QDebug>
#include <iostream>

// windows
#include <windows.h>

// user
#include "TcAdsDef.h"
#include "TcAdsAPI.h"

typedef struct _RoboMotionInfo {
    float posX;
    float posY;
    float posZ;
    float oriA;
    float oriB;
    float oriC;
    float graptryX;
    float graptryY;
}RoboMotionInfo;

class ads_client
{
public:
    ads_client();                                           //默认构造函数
    void AdsConnectLocal();                                 //行为：与本地进行ADS通信
    void AdsConnectRemote();                                //行为：与远端进行ADS通信
    void getRoboMotionInfo();                               //行为：读取机器人运动信息
    bool isConnected() const { return m_connected; }        //是否已成功建立ADS连接
    float getFloatVal(int offsetAddr);
    void setFloatVal(int offsetAddr, float setVal);
    short getIntVal(int offsetAddr);
    void setIntVal(int offsetAddr, short setVal);
    bool getBoolVal(int offsetAddr);
    // void getFloatVal(int offsetAddr, float& getVal);
    // void setFloatVal(int offsetAddr, float& setVal);
    // void getIntVal(int offsetAddr, short &getVal);
    // void setIntVal(int offsetAddr, short &setVal);
private:
    AmsAddr  Addr;//定义AMS地址变量
    PAmsAddr pAddr;//定义端口地址变量
    long nErr,nPort;
    USHORT  nAdsState;        //PLC状态信息
    USHORT  nDeviceState;
    bool m_connected = false; //ADS连接成功标志
};

#endif // ADS_CLIENT_H

