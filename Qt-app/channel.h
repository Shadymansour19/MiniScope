#ifndef CHANNEL_H
#define CHANNEL_H

#include <QDebug>
#include <vector>
#include <QString>
#include <qnamespace.h>
#include <qobject.h>
#include <numeric>
#include <cmath>
#include <math.h>
#include <QVector>
#include <QPointF>
#include <QtMath>
#include <complex>
#include <valarray>
#include <algorithm>
#include <QPushButton>
#include <QLineSeries>
#include <QValueAxis>
#include <QCheckBox>
#include <QToolButton>
#include "qLabeledUnitedSpinBox.h"
#include "qUnitedAxis.h"


// std::valarray-based Cooley-Tukey FFT
using Complex = std::complex<double>;
using CArray = std::valarray<Complex>;


class Channel {
public:
    const int MAX_POINTS = 50000;
    const int PEN_WIDTH  = 3;

    int id;
    bool isActive;
    QString label;
    QColor color;
    double displayMinY, displayMaxY, amplitude, dcLevel, frequency, period;
    QLabel *lblAmplitude, *lblDcLevel, *lblFreq, *lblPeriod;
    QToolButton *btnOnOff;
    QPen *pen;
    QIcon *icnOn, *icnOff;
    QLabeledUnitedSpinBox *dialPos, *dialRng;
    qUnitedAxis *axis;
    QLineSeries *series;
    QWidget *tabWidget;
    QVector<QPointF> pts;

    Channel(int id_, const QColor& color_, QChart *chart, QWidget *parentWidget = nullptr);
    void addPoints(const QVector<double>& times, const QVector<double>& vals);
    void analyze();
    void clear();
    void reset();
    void OnOffHandler();
    void autoScale();
    void updateDisplayMiniMax();
    void updateNumericDisplay();
    void enableUI();
    QString formatNum(double num);
};


#endif // CHANNEL_H
