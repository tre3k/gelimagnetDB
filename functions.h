#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QSqlQuery>
#include <QSqlDatabase>

#include <QMessageBox>

#include "structures.h"
#include "iqcustomplot.h"
#include "config.h"

enum{
  LinearAverageDirection_UP,
  LinearAverageDirection_DOWN,
  LinearAverageDirection_RIGHT,
  LinearAverageDirection_LEFT
};

struct sFindMaxHM{
    double yMax;
    double xMax;
    double xMaxHML;
    double xMaxHMR;
    double background;
};

void readDataDB(QSqlDatabase, sDataDB ,sData *, sOptions);
void plotData(iCasePlot2D *,sData *, sOptions);

void toCircle(double *,double *,double,double);
void average(sData *,double,double,double,double,double,double,QVector<double> *,QVector<double> *,QVector<double> *,bool,int);
int doubleToInt(double);

void paintSector(iQCustomPlot *,double,double,double,double,double,double);

void interpolate2up(double **,double **,int,int); // in, out
void interpolate(QSqlDatabase, sOptions ,sDataDB *,sData *,int);
void copydata(double **,double **,int,int);
double pix2Impulse(sData *,double);
void paintCross(iQCustomPlot *,sOptions, sData *, double , double , double);
void paintSmallCross(iQCustomPlot *, double, double);
void LinearAverage(QVector<double> *,QVector<double> *,QVector<double> *,sData *,
                   double,double,double,double,int);
int findIndexInList(QListWidget *);
sFindMaxHM findMaxHM(QVector<double> *, QVector<double> *);

#endif // FUNCTIONS_H
