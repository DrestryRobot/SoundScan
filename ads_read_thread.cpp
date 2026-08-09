#include "ads_read_thread.h"

extern RoboMotionInfo g_RoboMotionInfo;
extern ads_client adsClient;

float testVal[5];

using namespace std;

ads_read_thread::ads_read_thread(QObject *parent) : QThread{parent} {}

void ads_read_thread::Delay_MSec(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < _Timer )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void ads_read_thread::run()
{
    std::cout << std::fixed << std::setprecision(4);

    while (1) {
        testVal[0] = adsClient.getFloatVal(0);
        testVal[1] = adsClient.getFloatVal(4);
        qDebug() << testVal[0];
        qDebug() << testVal[1];
        Delay_MSec(10);
    }
}

