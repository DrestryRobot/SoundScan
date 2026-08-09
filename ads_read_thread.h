#ifndef ADS_READ_THREAD_H
#define ADS_READ_THREAD_H

// qt
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include "iostream"
#include <iomanip>

// user
#include "ads_client.h"



class ads_read_thread : public QThread
{
protected:
    void run() override;
public:
    explicit ads_read_thread(QObject *parent = nullptr);
private:
    void Delay_MSec(unsigned int msec);
};

#endif // ADS_READ_THREAD_H

