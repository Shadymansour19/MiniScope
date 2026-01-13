#include "serialthread.h"
#include <QDateTime>
#include <QTextStream>

SerialThread::SerialThread(QObject *parent, QString port) : QThread(parent) {
    serial = new QSerialPort(this);
    serial->setPortName(port);     // COM3 for windows - /dev/ttyUSB0 for linux
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
}

SerialThread::~SerialThread() {
    stop();
}

void SerialThread::stop() {
    running = false;
    if (serial->isOpen()) {
        serial->close();
    }
    wait();
}

void SerialThread::run() {
    serial->moveToThread(this);
    if (!serial->open(QIODevice::ReadWrite)) {
        qWarning("Failed to open serial port!");
        return;
    }

    serial->write("reset\n");
    running = true;
    QByteArray buffer;

    while (running) {
        if (serial->waitForReadyRead(50)) {
            buffer.append(serial->readAll());

            // assume each sample = "[ch-id] [time-stamp] [val]\n"
            while (buffer.contains('\n')) {
                int newlineIndex = buffer.indexOf('\n');
                QByteArray line = buffer.left(newlineIndex).trimmed();
                buffer.remove(0, newlineIndex + 1);

                if (line.isEmpty()) {
                    continue;
                }

                QTextStream ts(line);
                int ch = 0;
                double val = 0.0;
                double tim = 0.0;
                ts >> ch >> tim >> val;

                vals[ch].append(val);
                times[ch].append(tim);

                // tx 100 samples as chunck
                if (vals[0].size() >= 100) {
                    emit newSamples(0, vals[0], times[0]);
                    vals[0].clear();
                    times[0].clear();
                }
                if (vals[1].size() >= 100) {
                    emit newSamples(1, vals[1], times[1]);
                    vals[1].clear();
                    times[1].clear();
                }
            }
        }
    }

    serial->close();
}
