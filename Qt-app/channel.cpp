#include "channel.h"
#include <algorithm>


Channel::Channel(int id_, const QColor& color_, QChart* chart, fa::QtAwesome *awesome, QWidget *parentWidget)
    : id(id_), color(color_), isActive(true),
    displayMinY(-1.0), displayMaxY(1.0), amplitude(0), frequency(0), period(0) {
    label = QString::fromStdString("CH" + std::to_string(id));
    auto colorStyle = "color: " + color.name();

    lblAmplitude = new QLabel(parentWidget);
    lblFreq = new QLabel(parentWidget);
    lblPeriod = new QLabel(parentWidget);
    lblDcLevel = new QLabel(parentWidget);
    lblAmplitude->setStyleSheet(colorStyle);
    lblFreq->setStyleSheet(colorStyle);
    lblPeriod->setStyleSheet(colorStyle);
    lblDcLevel->setStyleSheet(colorStyle);
    QVariantMap onOpts;
    onOpts["color"] = color;
    onOpts["color-disabled"] = QColor("#555555");
    icnOn = awesome->icon(fa::fa_solid, fa::fa_wave_square, onOpts);

    QVariantMap offOpts;
    offOpts["color"] = QColor("#555555");
    icnOff = awesome->icon(fa::fa_solid, fa::fa_wave_square, offOpts);

    btnOnOff = new QToolButton(parentWidget);
    btnOnOff->setAutoRaise(true);
    btnOnOff->setIcon(icnOn);
    btnOnOff->setIconSize(QSize(48, 48));
    series = new QLineSeries();
    series->setName(label);
    pen = new QPen(color);
    pen->setWidth(PEN_WIDTH);
    series->setPen(*pen);
    axis = new qUnitedAxis(label, "V", chart, parentWidget);
    axis->setTitleBrush(QBrush(color));
    axis->setLinePenColor(color);
    axis->setLabelsColor(color);
    axis->setTickCount(5);

    dialRng = new QLabeledUnitedSpinBox(label + " Rng", "V", color, false, parentWidget);
    dialRng->setValue(2);

    dialPos = new QLabeledUnitedSpinBox(label + " Pos", "V", color, true, parentWidget);
    dialPos->setValue(0);

    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::StyledPanel);
    separator->setStyleSheet("border: none;");

    tabWidget = new QWidget(parentWidget);
    QGridLayout *tabLayout = new QGridLayout(tabWidget);
    tabLayout->addWidget(btnOnOff, 0, 0, 1, 2, Qt::AlignCenter);
    tabLayout->addWidget(dialRng, 1, 0, 1, 2, Qt::AlignCenter);
    tabLayout->addWidget(dialPos, 2, 0, 1, 2, Qt::AlignCenter);
    tabLayout->addWidget(separator, 3, 0, 1, 2);
    tabLayout->addWidget(new QLabel("Amp:"), 4, 0);                 tabLayout->addWidget(lblAmplitude, 4, 1);
    tabLayout->addWidget(new QLabel("DC offset:"), 5, 0);           tabLayout->addWidget(lblDcLevel, 5, 1);
    tabLayout->addWidget(new QLabel("Freq:"), 6, 0);                tabLayout->addWidget(lblFreq, 6, 1);
    tabLayout->addWidget(new QLabel("Period:"), 7, 0);              tabLayout->addWidget(lblPeriod, 7, 1);

    btnOnOff->setEnabled(false);
    dialRng->setEnabled(false);
    dialPos->setEnabled(false);
}

void Channel::enableUI() {
    btnOnOff->setEnabled(true);
    btnOnOff->setIcon(icnOn);
    btnOnOff->setChecked(true);
    dialPos->setEnabled(true);
    dialRng->setEnabled(true);
}

void Channel::addPoints(const QVector<double>& times, const QVector<double>& vals) {
    if (pts.size() + times.size() > Channel::MAX_POINTS) {
        pts.erase(pts.begin(), pts.begin() + (pts.size() + times.size() - Channel::MAX_POINTS));
    }
    for (size_t i = 0; i < vals.size(); i++) {
        pts.append(QPointF(times[i], vals[i]));
    }
}

void Channel::clear() {
    pts.clear();
    series->clear();
    displayMinY = -1;
    displayMaxY = 1;
    axis->setRange(displayMinY, displayMaxY);
}

void Channel::reset() {
    clear();
    btnOnOff->setEnabled(false);
    dialPos->setEnabled(false);
    dialRng->setEnabled(false);
}

void Channel::OnOffHandler() {
    isActive = !isActive;
    series->setVisible(isActive);
    axis->setVisible(isActive);
    axis->setTitleVisible(isActive);
    dialRng->setEnabled(isActive);
    dialPos->setEnabled(isActive);
    lblAmplitude->setVisible(isActive);
    lblFreq->setVisible(isActive);
    lblPeriod->setVisible(isActive);
    lblDcLevel->setVisible(isActive);
    if (isActive) {
        btnOnOff->setIcon(icnOn);
    } else {
        btnOnOff->setIcon(icnOff);
    }
}

void Channel::updateDisplayMiniMax() {
    auto rng = dialRng->getValue();
    auto pos = dialPos->getValue();
    displayMinY = pos - rng / 2.0;
    displayMaxY = pos + rng / 2.0;
    axis->setRange(displayMinY, displayMaxY);
}

QString Channel::formatNum(double num) {
    static QStringList factorsText = QStringList({"n", "µ", "m", "", "k", "M", "G"});
    static QList<double> factorsVal = QList({1e-9, 1e-6, 1e-3, 1e0, 1e3, 1e6, 1e9});
    if (num == 0.0) {
        return "0.0 ";
    }
    int i = 0;
    double absNum = abs(num);
    while (i+1 < factorsVal.length() && absNum / factorsVal[i] >= 1e3) {
        i++;
    }
    auto ret = QString::number(num / factorsVal[i], 'f', 1) + " " + factorsText[i];
    return ret;
}

void Channel::autoScale() {
    analyze();
    displayMinY = std::numeric_limits<double>::max();
    displayMaxY = std::numeric_limits<double>::min();
    for (auto& p : pts) {
        displayMinY = std::min(displayMinY, p.y());
        displayMaxY = std::max(displayMaxY, p.y());
    }

    auto pos = (displayMaxY + displayMinY) / 2;
    auto rng = (displayMaxY - displayMinY) * 1.4;
    if (rng == 0) {
        rng = 1.0;
    }

    displayMinY = pos - rng / 2;
    displayMaxY = pos + rng / 2;

    dialRng->setValue(rng);
    dialPos->setValue(pos);
    updateDisplayMiniMax();
}


// ========================================
// ======== FFT IMPLEMENTATION ============
// ========================================

void fft(CArray& x) {
    const size_t N = x.size();
    if (N <= 1) {
        return;
    }

    CArray even = x[std::slice(0, N/2, 2)];
    CArray  odd = x[std::slice(1, N/2, 2)];

    fft(even);
    fft(odd);

    for (size_t k = 0; k < N/2; ++k) {
        Complex t = std::polar(1.0, -2 * M_PI * k / N) * odd[k];
        x[k]       = even[k] + t;
        x[k + N/2] = even[k] - t;
    }
}

void Channel::analyze() {
    int N = pts.size();
    if (N < 2) {
        return;
    }

    int pow2 = 2;
    while (pow2 < N) {
        pow2 <<= 1;
    }
    N = pow2>>1;

    std::vector<double> time(N), vals(N);
    double maxVal = std::numeric_limits<double>::min();
    double minVal = std::numeric_limits<double>::max();
    double sumVals = 0;
    for (int i = 0; i < N; i++) {
        time[i] = pts[i].x();
        vals[i] = pts[i].y();
        maxVal = std::max(maxVal, pts[i].y());
        minVal = std::min(minVal, pts[i].y());
        sumVals += pts[i].y();
    }
    amplitude = (maxVal - minVal) / 2.0;

    dcLevel = sumVals / N;
    for (auto& v : vals) {
        v -= dcLevel;
    }

    // apply Hann window
    for (int i = 0; i < N; ++i) {
        vals[i] *= 0.5 * (1 - std::cos(2 * M_PI * i / (N - 1)));
    }

    // mean sampling interval
    double ts = (time[N-1] - time[0]) / (N-1);
    if (ts <= 0) {
        return;
    }

    // prepare complex array
    CArray data(N);
    for (int i = 0; i < N; ++i) {
        data[i] = Complex(vals[i], 0.0);
    }

    fft(data);

    // compute magnitudes
    std::vector<double> mag(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        mag[i] = std::abs(data[i]);
    }

    // compute frequency bins
    std::vector<double> freq(N / 2);
    double fs = 1.0 / ts;
    for (int i = 0; i < N / 2; ++i) {
        freq[i] = i * fs / N;
    }

    // find peak
    int peakIndex = std::distance(mag.begin(), std::max_element(mag.begin(), mag.end()));
    frequency = freq[peakIndex];
    period = 1.0 / frequency;

    lblAmplitude->setText(formatNum(amplitude) + "V");
    lblDcLevel->setText(formatNum(dcLevel) + "V");
    lblFreq->setText(formatNum(frequency) + "Hz");
    lblPeriod->setText(formatNum(period) + "S");
}
