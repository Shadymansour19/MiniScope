#include "qUnitedAxis.h"

qUnitedAxis::qUnitedAxis(const QString& title, const QString &unit, QChart *chart, QObject *parent) : QValueAxis(parent) {
    this->title = title;
    this->chart = chart;
    this->unit = unit;
    setTruncateLabels(true);
    QValueAxis::setTitleText(title + " (" + unit + ")");
    connect(this, &QValueAxis::rangeChanged, this, [=]() {
        // axisBeautify(chart);
    });
}

void qUnitedAxis::axisBeautify(QChart *chart) {
    qDebug() << "qUnitedAxis::axisBeautify";

    QStringList factorsText = QStringList({"n", "µ", "m", "", "k", "M", "G"});
    QList<double> factorsVal = QList({1e-9, 1e-6, 1e-3, 1e0, 1e3, 1e6, 1e9});
    auto maxVal = qMax(qAbs(min()), qAbs(max()));

    double factor = 1.0;
    for (int i = 0; i < factorsVal.length(); i++) {
        if (maxVal / factorsVal[i] < 1e3) {
            factor = factorsVal[i];
            QValueAxis::setTitleText(title + " (" + factorsText[i] + unit + ")");
            break;
        }
    }

    int ticks = tickCount();
    QStringList newLabels;
    QVector<qreal> tickPositions;

    for (int i = 0; i < ticks; ++i) {
        qreal val = min() + i * (max() - min()) / (ticks - 1);
        QString label = QString::number(val / factor, 'f', 1);
        newLabels << label;
        tickPositions << val;
    }

    QCategoryAxis *catAxis = new QCategoryAxis(chart);
    for (int i = 0; i < ticks; ++i) {
        catAxis->append(newLabels[i], tickPositions[i]);
        qDebug() << newLabels[i] << tickPositions[i];
    }

    catAxis->setTitleText(titleText());
    catAxis->setLinePen(linePen());
    catAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    catAxis->setGridLineVisible(true);

    auto align = alignment();
    chart->removeAxis(this);
    chart->addAxis(catAxis, align);
    qDebug() << "done";
}
