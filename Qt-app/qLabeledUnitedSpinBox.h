#ifndef QLABELEDDIAL_H
#define QLABELEDDIAL_H


#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QDebug>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QStringList>
#include "math.h"

class QLabeledUnitedSpinBox : public QWidget {
    Q_OBJECT

public:
    QLabeledUnitedSpinBox(const QString& labelText = "", const QString& unit = "", const QColor& color = Qt::black, bool acceptNegative = false, QWidget *parent = nullptr);
    ~QLabeledUnitedSpinBox();
    double getValue();
    void setValue(double val);
    void blockSignals(bool on);
    void setEnabled(bool on);

protected:
    QLabel *label;
    QDoubleSpinBox *spinBox;
    QComboBox *unitSelector;
    QStringList factorsText = QStringList({"n", "µ", "m", "", "k", "M", "G"});
    QList<double> factorsVal = QList({1e-9, 1e-6, 1e-3, 1e0, 1e3, 1e6, 1e9});

private:
    void valueChangeHandler();

signals:
    void valueChanged(double newValue);
};

#endif // QLABELEDDIAL_H
