#include "mainwindow.h"
#include <QRandomGenerator>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    currentState = stoped;
    recording = false;
    writeFile = nullptr;
    writeStream = nullptr;

    saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::getFileToSaveTo);

    loadShortcut = new QShortcut(QKeySequence::Open, this);
    connect(loadShortcut, &QShortcut::activated, this, &MainWindow::getFileToReadFrom);

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    QGridLayout *controlLayout = new QGridLayout();
    channelTabs = new QTabWidget();
    channelTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chart = new QChart();
    chart->setTheme(QChart::ChartThemeDark);
    chart->setAnimationOptions(QChart::NoAnimation);

    QLegend *legend = chart->legend();
    legend->setVisible(true);
    QFont legendFont = legend->font();
    legendFont.setPointSize(16);
    legend->setFont(legendFont);

    channels[0] = new Channel(1, Qt::yellow, chart, this);
    channels[1] = new Channel(2, QColor("#2FD9D4"), chart, this);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(chartView);
    mainLayout->addLayout(controlLayout);

    dispalyMinTime = 0;
    dispalyMaxTime = 2;
    dialTimeRng = new QLabeledUnitedDial("Time Rng", "S", Qt::white, false, this);
    dialTimeRng->setValue(dispalyMaxTime - dispalyMinTime);
    dialTimePos = new QLabeledUnitedDial("Time Pos", "S", Qt::white, false, this);
    dialTimePos->setValue(dispalyMaxTime);

    btnPlayStop = new QToolButton(this);
    btnPauseResume = new QToolButton(this);
    btnRefresh = new QToolButton(this);
    btnSaveToFile = new QToolButton(this);
    btnReadFromFile = new QToolButton(this);
    btnAuto = new QPushButton("AUTO");

    icnPlay = style()->standardIcon(QStyle::SP_MediaPlay);
    icnStop = style()->standardIcon(QStyle::SP_MediaStop);
    icnPause = style()->standardIcon(QStyle::SP_MediaPause);
    icnRefresh = style()->standardIcon(QStyle::SP_BrowserReload);
    icnSave = style()->standardIcon(QStyle::SP_DialogSaveButton);
    icnReadFile = style()->standardIcon(QStyle::SP_DirOpenIcon);

    btnPlayStop->setIcon(icnPlay);
    btnPlayStop->setToolTip("Start");
    btnPlayStop->setIconSize(QSize(24, 24));
    btnPlayStop->setAutoRaise(true);

    btnPauseResume->setIcon(icnPause);
    btnPauseResume->setToolTip("Pause");
    btnPauseResume->setIconSize(QSize(24, 24));
    btnPauseResume->setAutoRaise(true);

    btnRefresh->setIcon(icnRefresh);
    btnRefresh->setToolTip("Refresh");
    btnRefresh->setIconSize(QSize(24, 24));
    btnRefresh->setAutoRaise(true);

    btnSaveToFile->setIcon(icnSave);
    btnSaveToFile->setToolTip("Save incoming data to a file (Ctrl+S)");
    btnSaveToFile->setIconSize(QSize(24, 24));
    btnSaveToFile->setAutoRaise(true);

    btnReadFromFile->setIcon(icnReadFile);
    btnReadFromFile->setToolTip("Load plot from file (Ctrl+O)");
    btnReadFromFile->setIconSize(QSize(24, 24));
    btnReadFromFile->setAutoRaise(true);

    auto btnAutoFont = btnAuto->font();
    btnAutoFont.setBold(true);
    btnAuto->setFont(btnAutoFont);
    btnAuto->setStyleSheet("background-color: darkmagenta;");

    btnPauseResume->setEnabled(false);
    btnRefresh->setEnabled(false);
    btnSaveToFile->setEnabled(true);
    btnReadFromFile->setEnabled(true);
    btnAuto->setEnabled(false);
    dialTimeRng->setEnabled(false);
    dialTimePos->setEnabled(false);

    connect(btnPlayStop, &QToolButton::clicked, this, &MainWindow::btnPlayStopHandler);
    connect(btnPauseResume, &QToolButton::clicked, this, &MainWindow::btnPauseResumeHandler);
    connect(btnRefresh, &QToolButton::clicked, this, &MainWindow::btnRefreshHandler);
    connect(btnSaveToFile, &QToolButton::clicked, this, &MainWindow::getFileToSaveTo);
    connect(btnReadFromFile, &QToolButton::clicked, this, &MainWindow::getFileToReadFrom);
    connect(btnAuto, &QPushButton::clicked, this, &MainWindow::btnAutoHandler);
    connect(dialTimeRng, &QLabeledUnitedDial::valueChanged, this, &MainWindow::updateTimeRange);
    connect(dialTimePos, &QLabeledUnitedDial::valueChanged, this, &MainWindow::updateTimeRange);
    connect(channelTabs, &QTabWidget::currentChanged, this, &MainWindow::updateNumericDisplay);

    controlLayout->addWidget(btnPlayStop, 0, 0, Qt::AlignHCenter);
    controlLayout->addWidget(btnPauseResume, 0, 1, Qt::AlignHCenter);
    controlLayout->addWidget(btnRefresh, 0, 2, Qt::AlignHCenter);
    controlLayout->addWidget(btnSaveToFile, 0, 3, Qt::AlignHCenter);
    controlLayout->addWidget(btnReadFromFile, 0, 4, Qt::AlignHCenter);
    controlLayout->addWidget(btnAuto, 1, 0, 1, 5);
    controlLayout->addWidget(dialTimeRng, 2, 0, 1, 5, Qt::AlignHCenter);
    controlLayout->addWidget(dialTimePos, 3, 0, 1, 5, Qt::AlignHCenter);

    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::StyledPanel);
    separator->setStyleSheet("border: none;");
    controlLayout->addWidget(separator, 4, 0, 1, 5, Qt::AlignHCenter);

    controlLayout->addWidget(channelTabs, 5, 0, 1, 5, Qt::AlignHCenter);
    QTabBar *tabBar = channelTabs->tabBar();
    for (auto i = 0; i < NUM_CHS; i++) {
        channelTabs->addTab(channels[i]->tabWidget, channels[i]->label);
        tabBar->setTabTextColor(i, channels[i]->color);

        connect(channels[i]->btnOnOff, &QToolButton::clicked, this, [this, i]() {
            channels[i]->OnOffHandler();
        });
        connect(channels[i]->dialRng, &QLabeledUnitedDial::valueChanged, this, [this, i]() {
            channels[i]->updateDisplayMiniMax();
        });
        connect(channels[i]->dialPos, &QLabeledUnitedDial::valueChanged, this, [this, i]() {
            channels[i]->updateDisplayMiniMax();
        });
    }

    // Setup chart
    chart->addSeries(channels[0]->series);
    chart->addSeries(channels[1]->series);

    timeAxis = new qUnitedAxis("Time", "S", chart, this);

    chart->addAxis(timeAxis, Qt::AlignBottom);
    chart->addAxis(channels[0]->axis, Qt::AlignLeft);
    chart->addAxis(channels[1]->axis, Qt::AlignRight);

    for (auto& c : channels) {
        c->series->attachAxis(timeAxis);
        c->series->attachAxis(c->axis);
    }

    serialThread = new SerialThread(this);
    connect(serialThread, &SerialThread::newSamples, this, [=](int channel, QVector<double> values, QVector<double> times) {
        channels[channel]->addPoints(times, values);
        if (writeStream != nullptr) {
            for (int i = 0; i < values.size(); i++) {
                *writeStream << channel << " " << values[i] << " " << times[i] << "\n";
            }
        }
    });

    connect(&updatePlotTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    connect(&updateNumericDisplayTimer, &QTimer::timeout, this, &MainWindow::updateNumericDisplay);
    setCentralWidget(central);
}

MainWindow::~MainWindow() {
    serialThread->stop();
    writeFile->close();
}

void MainWindow::btnPlayStopHandler() {
    switch (currentState) {
    case stoped:
    case offline:
        currentState = running;
        btnPlayStop->setIcon(icnStop);
        btnPlayStop->setToolTip("Stop");
        channels[0]->enableUI();
        channels[1]->enableUI();
        btnPauseResume->setEnabled(true);
        btnRefresh->setEnabled(false);
        btnReadFromFile->setEnabled(false);
        loadShortcut->setEnabled(false);
        btnAuto->setEnabled(true);
        dialTimeRng->setEnabled(true);
        dialTimePos->setEnabled(false);
        updatePlotTimer.start(updatePlotInterval_ms);
        updateNumericDisplayTimer.start(updateNumericDispalyInterval_ms);
        serialThread->start();
        serialThread->reset();
        break;

    default:
        currentState = stoped;
        btnPlayStop->setIcon(icnPlay);
        btnPlayStop->setToolTip("Play");
        btnPauseResume->setIcon(icnPause);
        btnPauseResume->setToolTip("Pause");
        channels[0]->reset();
        channels[1]->reset();
        dispalyMinTime = 0;
        dispalyMaxTime = 2;
        timeAxis->setRange(dispalyMinTime, dispalyMaxTime);
        btnPauseResume->setEnabled(false);
        btnRefresh->setEnabled(false);
        btnAuto->setEnabled(false);
        dialTimeRng->setEnabled(false);
        dialTimePos->setEnabled(false);
        btnReadFromFile->setEnabled(true);
        loadShortcut->setEnabled(true);
        updatePlotTimer.stop();
        updateNumericDisplayTimer.stop();
        serialThread->stop();
        if (writeFile != nullptr) {
            writeFile->close();
            writeFile = nullptr;
            writeStream = nullptr;
        }
        break;
    }
}

void MainWindow::btnPauseResumeHandler() {
    switch (currentState) {
    case running:
        currentState = paused;
        btnPauseResume->setIcon(icnPlay);
        btnPauseResume->setToolTip("Resume");
        updatePlotTimer.stop();
        updateNumericDisplayTimer.stop();
        dialTimePos->setEnabled(true);
        btnRefresh->setEnabled(true);
        break;
    default:
        currentState = running;
        btnPauseResume->setIcon(icnPause);
        btnPauseResume->setToolTip("Pause");
        updatePlotTimer.start(updatePlotInterval_ms);
        updateNumericDisplayTimer.start(updateNumericDispalyInterval_ms);
        dialTimePos->setEnabled(false);
        btnRefresh->setEnabled(false);
        break;
    }
}

void MainWindow::btnRefreshHandler() {
    dispalyMaxTime = std::max(
        channels[0]->pts.empty() ? 0 : channels[0]->pts.back().x(),
        channels[1]->pts.empty() ? 0 : channels[1]->pts.back().x()
    );
    dialTimePos->setValue(dispalyMaxTime);
    updatePlot();
    updateNumericDisplay();
}

void MainWindow::getFileToSaveTo() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Choose output file",
        QDir::homePath(),
        "Text Files (*.txt);;All Files (*)"
    );

    if (!filePath.isEmpty()) {
        QFile *file = new QFile(filePath);
        if (file->open(QIODeviceBase::WriteOnly | QIODevice::Truncate)) {
            if (writeFile != nullptr) {
                writeFile->close();
            }
            writeFile = file;
            writeStream = new QTextStream(file);
        }
    }
}

void MainWindow::getFileToReadFrom() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Choose input file",
        QDir::homePath(),
        "Text Files (*.txt);;All Files (*)"
        );

    if (!filePath.isEmpty()) {
        QFile *file = new QFile(filePath);
        if (file->open(QIODevice::ReadOnly)) {
            readFile = file;
            readStream = new QTextStream(readFile);
            currentState = offline;
            btnAuto->setEnabled(true);
            dialTimeRng->setEnabled(true);
            dialTimePos->setEnabled(true);
            channels[0]->enableUI();
            channels[1]->enableUI();
            updatePlot();
            updateNumericDisplay();
        }
    }
}

void MainWindow::btnAutoHandler() {
    channels[0]->autoScale();
    channels[1]->autoScale();
    auto p1 = channels[0]->period;
    if (p1 == INFINITY || !channels[0]->isActive) {
        p1 = 1e-9;
    }
    auto p2 = channels[1]->period;
    if (p2 == INFINITY || !channels[1]->isActive) {
        p2 = 1e-9;
    }
    double timeRng = 0.0;
    if (p1 > 1e-9 && p2 > 1e-9) {
        timeRng = 2 * std::max(p1, p2);
    } else {
        timeRng = 4 * std::max(p1, p2);
    }
    dialTimeRng->setValue(timeRng);
    dispalyMinTime = std::max(0.0, dispalyMaxTime - dialTimeRng->getValue());
    timeAxis->setMin(dispalyMinTime);
}

void MainWindow::updateTimeRange() {
    dispalyMaxTime = dialTimePos->getValue();
    dispalyMinTime = std::max(0.0, dispalyMaxTime - dialTimeRng->getValue());
    timeAxis->setRange(dispalyMinTime, dispalyMaxTime);
    updatePlot();
}

void MainWindow::updateNumericDisplay() {
    channels[channelTabs->currentIndex()]->analyze();
}

void MainWindow::updatePlot() {
    if (currentState == offline) {
        int ch;
        double val, tim;
        QVector<double> vals[NUM_CHS], tims[NUM_CHS];
        setCursor(Qt::WaitCursor);
        channels[0]->clear();
        channels[1]->clear();
        readFile->reset();
        while (!readStream->atEnd()) {
            *readStream >> ch >> val >> tim;
            if (tim >= dispalyMinTime && tim <= dispalyMaxTime) {
                vals[ch].append(val);
                tims[ch].append(tim);
                if (vals[ch].size() >= 1000) {
                    channels[ch]->addPoints(tims[ch], vals[ch]);
                    vals[ch].clear();
                    tims[ch].clear();
                }
            }
        }
        channels[0]->addPoints(tims[0], vals[0]);
        channels[1]->addPoints(tims[1], vals[1]);
        setCursor(Qt::ArrowCursor);
    } else if (currentState == running) {
        dispalyMaxTime = std::max(
            channels[0]->pts.empty() ? 0 : channels[0]->pts.back().x(),
            channels[1]->pts.empty() ? 0 : channels[1]->pts.back().x()
        );
        dialTimePos->setValue(dispalyMaxTime);
    }

    channels[0]->series->replace(channels[0]->pts);
    channels[1]->series->replace(channels[1]->pts);
    dispalyMinTime = std::max(0.0, dispalyMaxTime - dialTimeRng->getValue());
    timeAxis->setRange(dispalyMinTime, dispalyMaxTime);
}
