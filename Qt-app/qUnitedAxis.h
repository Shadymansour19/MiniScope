#ifndef QUNITEDAXIS_H
#define QUNITEDAXIS_H


#include <QChart>
#include <QValueAxis>
#include <QCategoryAxis>
#include <QDebug>


class qUnitedAxis : public QValueAxis {
    Q_OBJECT

public:
    qUnitedAxis(const QString& title, const QString& unit, QChart *chart, QObject *parent = nullptr);

private:
    QString title;
    QChart *chart;
    QString unit;
    void axisBeautify(QChart *chart);
};


#endif // QUNITEDAXIS_H
