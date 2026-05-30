#include "mainwindow.h"
#include <QRandomGenerator>
#include <cmath>

MainWindow::MainWindow(const QString &port, QWidget *parent)
    : QMainWindow(parent) {

    awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();

    currentState = stopped;
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

    channels[0] = new Channel(1, Qt::yellow, chart, awesome, this);
    channels[1] = new Channel(2, QColor("#2FD9D4"), chart, awesome, this);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(chartView);
    mainLayout->addLayout(controlLayout);

    displayMinTime = 0;
    displayMaxTime = 2;
    dialTimeRng = new QLabeledUnitedSpinBox("Time Rng", "S", Qt::white, false, this);
    dialTimeRng->setValue(displayMaxTime - displayMinTime);
    dialTimePos = new QLabeledUnitedSpinBox("Time Pos", "S", Qt::white, false, this);
    dialTimePos->setValue(displayMaxTime);

    boxSerialPort = new QLineEdit(this);
    btnPlayStop = new QToolButton(this);
    btnPauseResume = new QToolButton(this);
    btnRefresh = new QToolButton(this);
    btnSaveToFile = new QToolButton(this);
    btnReadFromFile = new QToolButton(this);
    btnAuto = new QPushButton("AUTO");

    QVariantMap btnOpts;
    btnOpts["color"] = QColor(Qt::white);
    btnOpts["color-disabled"] = QColor("#555555");
    icnPlay    = awesome->icon(fa::fa_solid, fa::fa_play,        btnOpts);
    icnStop    = awesome->icon(fa::fa_solid, fa::fa_stop,        btnOpts);
    icnPause   = awesome->icon(fa::fa_solid, fa::fa_pause,       btnOpts);
    icnRefresh = awesome->icon(fa::fa_solid, fa::fa_sync,        btnOpts);
    icnSave    = awesome->icon(fa::fa_solid, fa::fa_save,        btnOpts);
    icnReadFile = awesome->icon(fa::fa_solid, fa::fa_folder_open, btnOpts);

    boxSerialPort->setPlaceholderText("Serial Port");
    if (port.isEmpty()) {
        boxSerialPort->setText("/dev/tty");
        for (auto &portInfo : QSerialPortInfo::availablePorts()) {
            if (portInfo.manufacturer() == "STMicroelectronics" || portInfo.description() == "STM32 Virtual ComPort") {
                boxSerialPort->setText(portInfo.systemLocation());
                break;
            }
            // qDebug() << "Port Name:" << portInfo.portName();
            // qDebug() << "System Location:" << portInfo.systemLocation();
            // qDebug() << "Description:" << portInfo.description();
            // qDebug() << "Manufacturer:" << portInfo.manufacturer();
            // qDebug() << "Vendor Identifier:" << (portInfo.hasVendorIdentifier() ? QByteArray::number(portInfo.vendorIdentifier(), 16) : "N/A");
            // qDebug() << "Product Identifier:" << (portInfo.hasProductIdentifier() ? QByteArray::number(portInfo.productIdentifier(), 16) : "N/A");
            // qDebug() << "-----------------------------------------";
        }
    } else {
        boxSerialPort->setText(port);
    }
    boxSerialPort->setToolTip("Serial Port");
    boxSerialPort->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid;"
        "  border-radius: 5px;"
        "  padding: 4px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid green;"
        "}"
    );

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
    btnAuto->setStyleSheet(
        "QPushButton {"
        "  background-color: darkmagenta;"
        "  border-radius: 10px;"
        "  padding: 4px;"
        "}"
    );

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
    connect(dialTimeRng, &QLabeledUnitedSpinBox::valueChanged, this, &MainWindow::updateTimeRange);
    connect(dialTimePos, &QLabeledUnitedSpinBox::valueChanged, this, &MainWindow::updateTimeRange);
    connect(channelTabs, &QTabWidget::currentChanged, this, &MainWindow::updateNumericDisplay);

    controlLayout->addWidget(boxSerialPort, 0, 0, 1, 5);
    controlLayout->addWidget(btnPlayStop, 1, 0, Qt::AlignHCenter);
    controlLayout->addWidget(btnPauseResume, 1, 1, Qt::AlignHCenter);
    controlLayout->addWidget(btnRefresh, 1, 2, Qt::AlignHCenter);
    controlLayout->addWidget(btnSaveToFile, 1, 3, Qt::AlignHCenter);
    controlLayout->addWidget(btnReadFromFile, 1, 4, Qt::AlignHCenter);
    controlLayout->addWidget(btnAuto, 2, 0, 1, 5);
    controlLayout->addWidget(dialTimeRng, 3, 0, 1, 5, Qt::AlignHCenter);
    controlLayout->addWidget(dialTimePos, 4, 0, 1, 5, Qt::AlignHCenter);

    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::StyledPanel);
    separator->setStyleSheet("border: none;");
    controlLayout->addWidget(separator, 5, 0, 1, 5, Qt::AlignHCenter);

    controlLayout->addWidget(channelTabs, 6, 0, 1, 5, Qt::AlignHCenter);
    QTabBar *tabBar = channelTabs->tabBar();
    for (auto i = 0; i < NUM_CHS; i++) {
        channelTabs->addTab(channels[i]->tabWidget, channels[i]->label);
        tabBar->setTabTextColor(i, channels[i]->color);

        connect(channels[i]->btnOnOff, &QToolButton::clicked, this, [this, i]() {
            channels[i]->OnOffHandler();
        });
        connect(channels[i]->dialRng, &QLabeledUnitedSpinBox::valueChanged, this, [this, i]() {
            channels[i]->updateDisplayMiniMax();
        });
        connect(channels[i]->dialPos, &QLabeledUnitedSpinBox::valueChanged, this, [this, i]() {
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

    serialThread = new QThread(this);
    serialReader = new SerialReader();
    serialReader->moveToThread(serialThread);
    connect(this, &MainWindow::serialEnable, serialReader, &SerialReader::start);
    connect(this, &MainWindow::serialDisable, serialReader, &SerialReader::stop);
    connect(serialThread, &QThread::destroyed, serialReader, &SerialReader::stop);
    connect(serialThread, &QThread::finished, serialReader, &SerialReader::stop);
    connect(serialReader, &SerialReader::newSamples, this, [=](int channel, QVector<double> values, QVector<double> times) {
        if (currentState == stopped) {
            return;
        }
        if (channel < 0 || channel >= NUM_CHS) {
            return;
        }

        channels[channel]->addPoints(times, values);
        if (writeStream != nullptr) {
            for (int i = 0; i < values.size(); i++) {
                *writeStream << channel << " " << values[i] << " " << times[i] << "\n";
            }
        }
    });

    serialThread->start();

    connect(&updatePlotTimer, &QTimer::timeout, this, &MainWindow::updatePlot);
    connect(&updateNumericDisplayTimer, &QTimer::timeout, this, &MainWindow::updateNumericDisplay);
    setCentralWidget(central);

    if (!port.isEmpty()) {
        QTimer::singleShot(0, this, &MainWindow::btnPlayStopHandler);
    }
}

MainWindow::~MainWindow() {
    emit serialDisable();
    serialThread->quit();
    if (serialThread->wait(3000) == false) {
        serialThread->terminate();
    }
    if (writeFile) {
        writeFile->close();
    }
}


void MainWindow::btnPlayStopHandler() {
    switch (currentState) {
    case stopped:
    case offline:
        currentState = running;
        btnPlayStop->setIcon(icnStop);
        btnPlayStop->setToolTip("Stop");
        channels[0]->enableUI();
        channels[1]->enableUI();
        serialReader->setPort(boxSerialPort->text());
        btnPauseResume->setEnabled(true);
        btnRefresh->setEnabled(false);
        btnReadFromFile->setEnabled(false);
        loadShortcut->setEnabled(false);
        btnAuto->setEnabled(true);
        dialTimeRng->setEnabled(true);
        dialTimePos->setEnabled(false);
        updatePlotTimer.start(updatePlotInterval_ms);
        updateNumericDisplayTimer.start(updateNumericDisplayInterval_ms);
        emit serialEnable();
        break;

    default:
        currentState = stopped;
        btnPlayStop->setIcon(icnPlay);
        btnPlayStop->setToolTip("Play");
        btnPauseResume->setIcon(icnPause);
        btnPauseResume->setToolTip("Pause");
        channels[0]->reset();
        channels[1]->reset();
        displayMinTime = 0;
        displayMaxTime = 2;
        timeAxis->setRange(displayMinTime, displayMaxTime);
        btnPauseResume->setEnabled(false);
        btnRefresh->setEnabled(false);
        btnAuto->setEnabled(false);
        dialTimeRng->setEnabled(false);
        dialTimePos->setEnabled(false);
        btnReadFromFile->setEnabled(true);
        loadShortcut->setEnabled(true);
        updatePlotTimer.stop();
        updateNumericDisplayTimer.stop();
        emit serialDisable();
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
        updateNumericDisplayTimer.start(updateNumericDisplayInterval_ms);
        dialTimePos->setEnabled(false);
        btnRefresh->setEnabled(false);
        break;
    }
}

void MainWindow::btnRefreshHandler() {
    displayMaxTime = std::max(
        channels[0]->pts.empty() ? 0 : channels[0]->pts.back().x(),
        channels[1]->pts.empty() ? 0 : channels[1]->pts.back().x()
    );
    dialTimePos->setValue(displayMaxTime);
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
    displayMinTime = std::max(0.0, displayMaxTime - dialTimeRng->getValue());
    timeAxis->setMin(displayMinTime);
}

void MainWindow::updateTimeRange() {
    displayMaxTime = dialTimePos->getValue();
    displayMinTime = std::max(0.0, displayMaxTime - dialTimeRng->getValue());
    timeAxis->setRange(displayMinTime, displayMaxTime);
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
            if (ch < 0 || ch >= NUM_CHS) {
                continue;
            }
            if (tim >= displayMinTime && tim <= displayMaxTime) {
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
        displayMaxTime = std::max(
            channels[0]->pts.empty() ? 0 : channels[0]->pts.back().x(),
            channels[1]->pts.empty() ? 0 : channels[1]->pts.back().x()
        );
        dialTimePos->setValue(displayMaxTime);
    }

    channels[0]->series->replace(channels[0]->pts);
    channels[1]->series->replace(channels[1]->pts);
    displayMinTime = std::max(0.0, displayMaxTime - dialTimeRng->getValue());
    timeAxis->setRange(displayMinTime, displayMaxTime);
}
