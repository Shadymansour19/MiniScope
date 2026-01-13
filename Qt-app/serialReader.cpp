#include "serialReader.h"
#include <QDateTime>
#include <QTextStream>

SerialReader::SerialReader(QString port) {
    serial = new QSerialPort(this);
    serial->setPortName(port);
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
}

SerialReader::~SerialReader() {
    stop();
}

void SerialReader::setPort(QString port) {
    serial->setPortName(port);
}

void SerialReader::stop() {
    running = false;
}

void SerialReader::run() {
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

            // each sample = "[ch-id] [time-stamp] [val]\n"
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
