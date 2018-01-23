#include "dataselector.h"
#include "ui_dataselector.h"

dataselector::dataselector(QSqlDatabase *database, QVector<sDataDB> *inDataList, sOptions *opt, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dataselector)
{
    ui->setupUi(this);

    plot = new iCasePlot2D;
    ui->plotLayout->addWidget(plot);

    for(int i=0;i<inDataList->count();i++){
        ui->listWidget->addItem(inDataList->at(i).filename);
    }

    selectorDataList = inDataList;
    options = opt;
    db = database;
    oDataList = new QVector<sDataDB>;

    connect(ui->listWidget,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_selectList(int)));
}

dataselector::~dataselector()
{
    delete ui;
}

void dataselector::slot_selectList(int index){
    sData rdata;

    readDataDB(*db,(selectorDataList->at(index)),&rdata,*options);
    plotData(plot,&rdata,*options);

    ui->label_distanceSD->setText(QString::number(rdata.distance_SD)+" m");
    ui->label_timeExposition->setText(QString::number(rdata.time_exposition)+" s");
    ui->label_magneticFild->setText(QString::number(rdata.fild_T)+" T");
    ui->label_temperature->setText(QString::number(rdata.temp_K)+" K");
    ui->label_wavelenght->setText(QString::number(rdata.wavelenght)+" A");

    for(int i=0;i<rdata.n_x;i++) delete [] rdata.data[i];
    return;

}

void dataselector::on_buttonBox_accepted()
{
    for(int i=0;i<ui->listWidget->count();i++){
        if(ui->listWidget->item(i)->isSelected()){
            oDataList->append(selectorDataList->at(i));
        }
    }

    emit signal_selected(oDataList);
}
