#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H


#include <QObject>
#include <QSerialPort>
#include <QVector>
#include <QByteArray>


class SerialReader : public QObject {
    Q_OBJECT
public:
    static const int NUM_CHS = 2;

    SerialReader(QString port = "/dev/pts/2");
    ~SerialReader();

    void setPort(QString port);

public slots:
    void start();   // opens the port and sends "reset\n"; reads are driven by readyRead
    void stop();    // closes the port; safe to call repeatedly

signals:
    void newSamples(int channel, QVector<double> values, QVector<double> times);

private slots:
    void onReadyRead();

private:
    void flush(int ch);

    QSerialPort *serial;
    QByteArray buffer;
    QVector<double> vals[NUM_CHS];
    QVector<double> times[NUM_CHS];
};


#endif // SERIALTHREAD_H
