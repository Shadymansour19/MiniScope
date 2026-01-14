#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <math.h>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QTimer>
#include <QPushButton>
#include <QToolButton>
#include <QTabWidget>
#include <QFileDialog>
#include <QShortcut>
#include <QCursor>
#include "channel.h"
#include <QThread>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QLineEdit>
#include "serialReader.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    const static int NUM_CHS = 2;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    const int updatePlotInterval_ms = 30;
    const int updateNumericDispalyInterval_ms = 1000;

    enum State {
        stoped,
        running,
        paused,
        offline
    };

signals:
    void serialEnable();
    void serialDisable();

private slots:
    void btnPlayStopHandler();
    void btnPauseResumeHandler();
    void btnRefreshHandler();
    void getFileToSaveTo();
    void getFileToReadFrom();
    void updatePlot();
    void updateNumericDisplay();
    void btnAutoHandler();
    void updateTimeRange();

private:
    double dispalyMinTime, dispalyMaxTime;
    State currentState;
    bool recording;
    QFile *writeFile, *readFile;
    QTextStream *writeStream, *readStream;
    QThread *serialThread;
    SerialReader *serialReader;
    QLabeledUnitedSpinBox *dialTimeRng, *dialTimePos;
    QChart *chart;
    QChartView *chartView;
    QValueAxis *timeAxis;
    QTimer updatePlotTimer, updateNumericDisplayTimer;
    QLineEdit *boxSerialPort;
    QToolButton *btnPlayStop, *btnPauseResume, *btnRefresh, *btnSaveToFile, *btnReadFromFile;
    QPushButton *btnAuto;
    QIcon icnPlay, icnStop, icnPause, icnRefresh, icnSave, icnReadFile;
    QShortcut *saveShortcut, *loadShortcut;
    QTabWidget *channelTabs;
    Channel *(channels[NUM_CHS]);
    void onNewSamples(int channel, QVector<double> values, QVector<double> times);
};


#endif // MAINWINDOW_H
