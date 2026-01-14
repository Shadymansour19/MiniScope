#include "serialReader.h"
#include <QDateTime>
#include <QTextStream>

SerialReader::SerialReader(QString port) {
    serial = new QSerialPort(this);
    serial->setPortName(port);
}

SerialReader::~SerialReader() {
    stop();
}

void SerialReader::setPort(QString port) {
    serial->setPortName(port);
}

void SerialReader::stop() {
    qDebug() << "stoping";
    running = false;
    alive = false;
}

void SerialReader::start() {
    qDebug() << "starting";
    if (!serial->open(QIODevice::ReadWrite)) {
        qWarning("Failed to open serial port!");
        qDebug() << serial->isOpen() ;
        return;
    }
    serial->write("reset\n");
    running = true;
}

void SerialReader::loop() {
    qDebug() << "loop...";
    QEventLoop lcoalLoop;
    QByteArray buffer;
    while (alive) {
        lcoalLoop.processEvents();
        if (running && serial->waitForReadyRead(50)) {
            buffer.append(serial->readAll());

            // each sample = "[ch-id] [time-stamp] [val]\n"
            while (buffer.contains('\n') && running) {
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

                lcoalLoop.processEvents();
            }
        }
    }

    if (serial->isOpen()) {
        serial->close();
    }
}
