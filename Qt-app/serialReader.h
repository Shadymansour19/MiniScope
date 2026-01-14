#ifndef SERIAALTHREAD_H
#define SERIAALTHREAD_H


#include <QThread>
#include <QSerialPort>
#include <QVector>
#include <QPointF>
#include <QElapsedTimer>
#include <QEventLoop>
// #include "mainwindow.h"


class SerialReader : public QObject {
    Q_OBJECT
public:
    SerialReader(QString port = "/dev/pts/2");
    ~SerialReader();

    void setPort(QString port);
    void stop();
    void start();
    void loop();

signals:
    void newSamples(int channel, QVector<double> values, QVector<double> times);

private:
    QSerialPort *serial;
    bool running = false;
    bool alive = true;
    QVector<double> vals[2];
    QVector<double> times[2];
};


#endif // SERIAALTHREAD_H
