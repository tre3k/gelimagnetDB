#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QFile>
#include <QXmlStreamReader>

#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "iqcustomplot.h"
#include "dialogoptions.h"
#include "structures.h"
#include "dbexplorer.h"
#include "functions.h"
#include "dataselector.h"

#include "config.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QSqlDatabase db;

    sOptions mainOptions;
    DialogOptions *dialogOptions;
    iCasePlot2D *plotView2D;
    DBExplorer *dbexplorer;

    iCasePlot2D *plotTscan2D;
    iQCustomPlot *plotTscan;

    iCasePlot2D *plotKs2D;
    iQCustomPlot *plotKsX;
    QVector<double> vKsAverageXxR;
    QVector<double> vKsAverageXyR;
    QVector<double> vKsAverageXerrR;
    QVector<double> vKsAverageXxL;
    QVector<double> vKsAverageXyL;
    QVector<double> vKsAverageXerrL;
    iQCustomPlot *plotKsY;
    QVector<double> vKsAverageYx;
    QVector<double> vKsAverageYy;
    QVector<double> vKsAverageYerr;


    QProgressBar *globalProgressBar;

    int globalSampleID = 0;
    QVector<sDataDB> globalDataList;

    dataselector *dSelector;

    QVector<sDataDB> vDataTscanDB;
    QVector<sData> vDataTscan;

    QVector<sDataDB> vDataKsDB;
    QVector<sData> vDataKs;


private:
    Ui::MainWindow *ui;

private slots:
    void slot_getOptions(sOptions);
    void slot_clickDBExplorer_data(int);

    void slot_TscanListAdd(QVector<sDataDB> *);
    void slot_TscanListClick(int);

    void slot_KsListAdd(QVector<sDataDB> *);
    void slot_KsListClick(int);
    void slot_KsCenterChanged(double);

    void on_actionE_xit_triggered();
    void on_actionO_ptions_triggered();

    void on_actionconnect_triggered();

    void on_actiondisconnect_triggered();


    void on_pushButton_clicked();

    void on_pushButtonTscanAdd_clicked();

    void on_pushButtonTscanClr_clicked();

    void on_pushButtonTscanMinus_clicked();

    void on_pushButtonTscanStart_clicked();

    void on_pushButtonKs_add_clicked();

    void on_pushButton_Ks_minus_clicked();

    void on_pushButton_Ks_clr_clicked();

    void on_pushButtonKsInterpolate_clicked();

    void on_pushButtonKsCenterOk_clicked();

    void on_pushButtonKsAverage_clicked();

signals:
    void signal_sendOptions(sOptions);
    void signal_dbConnected(QSqlDatabase);
    void signal_dbDisconnect();
};

#endif // MAINWINDOW_H
