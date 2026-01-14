#include "qLabeledUnitedSpinBox.h"

QLabeledUnitedSpinBox::QLabeledUnitedSpinBox(const QString& labelText, const QString& unit, const QColor& color, bool acceptNegative, QWidget *parent) : QWidget(parent) {
    for (auto i = 0; i < factorsText.length(); i++) {
        factorsText[i] += unit;
    }

    label = new QLabel(labelText, this);
    spinBox = new QDoubleSpinBox(this);
    unitSelector = new QComboBox(this);

    unitSelector->addItems(factorsText);

    spinBox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    spinBox->setDecimals(1);
    // spinBox->setFixedSize(80, 30);
    if (acceptNegative) {
        spinBox->setRange(-1000, 1000);
    } else {
        spinBox->setRange(0, 1000);
    }

    auto colorStyle = "color: " + color.name();
    label->setStyleSheet(colorStyle);
    spinBox->setStyleSheet(colorStyle);
    unitSelector->setStyleSheet(colorStyle);

    QVBoxLayout *allLayout = new QVBoxLayout(this);
    QHBoxLayout *valLayout = new QHBoxLayout();
    allLayout->addWidget(label, 0, Qt::AlignBottom | Qt::AlignHCenter);
    allLayout->addLayout(valLayout);
    valLayout->addWidget(spinBox, 0, Qt::AlignRight);
    valLayout->addWidget(unitSelector, 0, Qt::AlignLeft);

    connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QLabeledUnitedSpinBox::valueChangeHandler);
    connect(unitSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
        emit valueChanged(getValue());
    });
}

QLabeledUnitedSpinBox::~QLabeledUnitedSpinBox() {
    delete label;
    delete spinBox;
    delete unitSelector;
}

void QLabeledUnitedSpinBox::blockSignals(bool on) {
    spinBox->blockSignals(on);
    unitSelector->blockSignals(on);
}

void QLabeledUnitedSpinBox::setEnabled(bool on) {
    spinBox->setEnabled(on);
    unitSelector->setEnabled(on);
}

void QLabeledUnitedSpinBox::valueChangeHandler() {
    blockSignals(true);
    auto val = spinBox->value();
    auto i = unitSelector->currentIndex();
    auto absVal = abs(val);

    if (absVal == 1e3 && i+1 < unitSelector->count()) {
        val /= 1e3;
        i++;
    } else if (absVal < 1 && i > 0) {
        val *= 1e3;
        i--;
    }

    unitSelector->setCurrentIndex(i);
    spinBox->setValue(val);
    blockSignals(false);
    emit valueChanged(val * factorsVal[i]);
}

double QLabeledUnitedSpinBox::getValue() {
    return spinBox->value() * factorsVal[unitSelector->currentIndex()];
}

void QLabeledUnitedSpinBox::setValue(double val) {
    for (int i = 0; i < factorsVal.length(); i++) {
        if (val / factorsVal[i] < 1e3) {
            blockSignals(true);
            unitSelector->setCurrentIndex(i);
            val /= factorsVal[i];
            spinBox->setValue(val);
            blockSignals(false);
            break;
        }
    }
}
