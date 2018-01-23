#ifndef DATASELECTOR_H
#define DATASELECTOR_H

#include <QDialog>
#include <QVector>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "iqcustomplot.h"
#include "functions.h"
#include <structures.h>

namespace Ui {
class dataselector;
}

class dataselector : public QDialog
{
    Q_OBJECT

public:
    explicit dataselector(QSqlDatabase *, QVector<sDataDB> *, sOptions *, QWidget *parent = 0);
    ~dataselector();

    iCasePlot2D *plot;


public slots:
    void slot_selectList(int);


private slots:
    void on_buttonBox_accepted();

private:
    QSqlDatabase *db;
    sOptions *options;
    QVector<sDataDB> *selectorDataList;
    QVector<sDataDB> *oDataList;
    Ui::dataselector *ui;

signals:
    void signal_selected(QVector<sDataDB> *);
};

#endif // DATASELECTOR_H
