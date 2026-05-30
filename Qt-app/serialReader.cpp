#include "serialReader.h"
#include <QTextStream>
#include <QDebug>

SerialReader::SerialReader(QString port) {
    serial = new QSerialPort(this);
    serial->setPortName(port);
    serial->setBaudRate(QSerialPort::Baud115200);
    connect(serial, &QSerialPort::readyRead, this, &SerialReader::onReadyRead);
}

SerialReader::~SerialReader() {
    stop();
}

void SerialReader::setPort(QString port) {
    serial->setPortName(port);
}

void SerialReader::start() {
    if (serial->isOpen()) {
        return;
    }
    if (!serial->open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open serial port" << serial->portName() << ":" << serial->errorString();
        return;
    }
    // start from a clean slate so a stop/start cycle does not leak stale samples
    buffer.clear();
    for (int ch = 0; ch < NUM_CHS; ch++) {
        vals[ch].clear();
        times[ch].clear();
    }
    serial->write("reset\n");
}

void SerialReader::stop() {
    if (serial->isOpen()) {
        serial->close();
    }
    buffer.clear();
}

void SerialReader::onReadyRead() {
    buffer.append(serial->readAll());

    // each sample line = "[ch-id] [time-stamp] [val]\n"; a trailing partial line
    // (no '\n' yet) is left in the buffer for the next readyRead.
    int newlineIndex;
    while ((newlineIndex = buffer.indexOf('\n')) != -1) {
        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        QTextStream ts(line);
        int ch = -1;
        double tim = 0.0;
        double val = 0.0;
        ts >> ch >> tim >> val;

        // drop malformed lines and out-of-range channel ids (prevents OOB writes)
        if (ts.status() != QTextStream::Ok || ch < 0 || ch >= NUM_CHS) {
            continue;
        }

        vals[ch].append(val);
        times[ch].append(tim);

        // tx 100 samples as a chunk
        if (vals[ch].size() >= 100) {
            flush(ch);
        }
    }
}

void SerialReader::flush(int ch) {
    if (vals[ch].isEmpty()) {
        return;
    }
    emit newSamples(ch, vals[ch], times[ch]);
    vals[ch].clear();
    times[ch].clear();
}
