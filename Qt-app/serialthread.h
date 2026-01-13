#ifndef SERIAALTHREAD_H
#define SERIAALTHREAD_H


#include <QThread>
#include <QSerialPort>
#include <QVector>
#include <QPointF>
#include <QElapsedTimer>
// #include "mainwindow.h"


class SerialThread : public QThread {
    Q_OBJECT
public:
    SerialThread(QObject *parent = nullptr, QString port = "");
    ~SerialThread();

    void stop();
    void reset();

signals:
    void newSamples(int channel, QVector<double> values, QVector<double> times);

protected:
    void run() override;

private:
    QSerialPort serial;
    bool running = false;
    QVector<double> vals[2];
    QVector<double> times[2];
};


#endif // SERIAALTHREAD_H
