#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QString>

struct sOptions{
    QString userNameDB;
    QString hostNameDB;
    QString passwordDB;
    QString database;

    QString workDirecotry;

    int axies2DPlots;

    enum TYPE_OF_AXIES{
       InPixels,
       InImpulseNm,
       InImpulseA
    };
};

struct sData{
    QString filename;
    int id;
    int monitor1;
    int monitor2;

    int step_interpolate;
    double centerX,centerY,centerAngle;
    double KsX1,KsX2,KsY1,KsY2;
    double KsValue;

    double temp_K;
    double fild_T;
    double time_exposition;
    double distance_SD;

    int flag;
    int n_x;
    int n_y;
    double resolution;
    double wavelenght;

    double **data;
};

struct sDataDB{
    int id;
    int id_proposal;
    int id_sample;
    int id_detector;
    int id_prefix;

    double temp_K;
    double fild_T;

    QString filename;
};

struct sProposalDB{
    int id;

    QString name;
    QString start_date;
};

struct sSampleDB{
    int id;
    int concentrate;
    int id_proposal;

    QString sample_name;
    QString comments;
};

#endif // STRUCTURES_H
