#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* read config */

    QFile file_config(CONFIG_FILE);
    if(file_config.open(QIODevice::ReadOnly)){
        QXmlStreamReader xml(&file_config);

        while(!xml.atEnd()){
            xml.readNext();
            if(xml.isStartElement()){
                if(xml.name()=="loginDB") mainOptions.userNameDB = xml.readElementText();
                if(xml.name()=="hostnameDB") mainOptions.hostNameDB = xml.readElementText();
                if(xml.name()=="passDB") mainOptions.passwordDB = xml.readElementText();
                if(xml.name()=="database") mainOptions.database = xml.readElementText();
                if(xml.name()=="workdirectory") mainOptions.workDirecotry = xml.readElementText();
                if(xml.name()=="plot_axies") mainOptions.axies2DPlots = xml.readElementText().toInt();
            }
        }

        file_config.close();
    }

    /* create dialog Options */
    dialogOptions = new DialogOptions();

    /* DB Explorer Widget */

    dbexplorer = new DBExplorer();
    ui->layoutDBExplorer->addWidget(dbexplorer);

    /* create plot View2D */
    plotView2D = new iCasePlot2D();
    ui->layoutViewPlot->addWidget(plotView2D);

    /* create plots T-scan */
    plotTscan = new iQCustomPlot;
    plotTscan2D = new iCasePlot2D;
    ui->layoutPlotTscan->addWidget(plotTscan);
    ui->layoutPlotTscan2D->addWidget(plotTscan2D);

    /* create plots Ks */
    plotKsX = new iQCustomPlot;
    plotKsY = new iQCustomPlot;
    plotKs2D = new iCasePlot2D;
    ui->layoutPlotKs2D->addWidget(plotKs2D);
    ui->layoutPlotKsX->addWidget(plotKsX);
    ui->layoutPlotKsY->addWidget(plotKsY);

    /* progressvBar in status Bar */
    globalProgressBar = new QProgressBar(ui->statusBar);
    globalProgressBar->setMaximumSize(100, 15);
    globalProgressBar->setAlignment(Qt::AlignRight);
    ui->statusBar->addPermanentWidget(globalProgressBar);


    db = QSqlDatabase::addDatabase(SQLTYPE);
    ui->actiondisconnect->setEnabled(false);

    this->setCentralWidget(ui->tabWidget);



    connect(dialogOptions,SIGNAL(signal_sendOptions(sOptions)),
            this,SLOT(slot_getOptions(sOptions)));
    connect(this,SIGNAL(signal_sendOptions(sOptions)),
            dialogOptions,SLOT(slot_getOptions(sOptions)));

    connect(this,SIGNAL(signal_dbConnected(QSqlDatabase)),
            dbexplorer,SLOT(slot_connected(QSqlDatabase)));
    connect(this,SIGNAL(signal_dbDisconnect()),
            dbexplorer,SLOT(slot_disconnect()));
    connect(dbexplorer,SIGNAL(signal_clickDataList(int)),
            this,SLOT(slot_clickDBExplorer_data(int)));

    connect(ui->spinBoxKsCenterX,SIGNAL(valueChanged(double)),
            this,SLOT(slot_KsCenterChanged(double)));
    connect(ui->spinBoxKsCenterY,SIGNAL(valueChanged(double)),
            this,SLOT(slot_KsCenterChanged(double)));
    connect(ui->spinBoxKsAngle,SIGNAL(valueChanged(double)),
            this,SLOT(slot_KsCenterChanged(double)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionE_xit_triggered()
{
    exit(0);
}

void MainWindow::on_actionO_ptions_triggered()
{
    dialogOptions->show();
    emit signal_sendOptions(mainOptions);
}


void MainWindow::on_actionconnect_triggered()
{
    db.setHostName(mainOptions.hostNameDB);
    db.setUserName(mainOptions.userNameDB);
    db.setPassword(mainOptions.passwordDB);
    db.setDatabaseName(mainOptions.database);

    if(!db.open()){
        QMessageBox::critical(0,"Error connection!","Error connect to "
                              +mainOptions.hostNameDB+" at "
                              +mainOptions.database+ " database.");
        return;
    }
    ui->actionconnect->setEnabled(false);
    ui->actiondisconnect->setEnabled(true);
    ui->statusBar->showMessage("connection to the database is established");

    dbexplorer->ListData->setEnabled(true);
    dbexplorer->ListSamples->setEnabled(true);
    dbexplorer->ListProposals->setEnabled(true);

    emit signal_dbConnected(db);
    return;
}

void MainWindow::on_actiondisconnect_triggered()
{
    db.close();
    ui->actionconnect->setEnabled(true);
    ui->actiondisconnect->setEnabled(false);
    ui->statusBar->showMessage("disconnected from database");
    emit signal_dbDisconnect();
}


void MainWindow::slot_getOptions(sOptions opt){
    mainOptions = opt;
}

void MainWindow::slot_clickDBExplorer_data(int index){
    sData data;
    //qDebug () << dbexplorer->vDatas.at(index).filename;
    readDataDB(db, dbexplorer->vDatas.at(index), &data, mainOptions);
    plotData(plotView2D,&data, mainOptions);

    ui->label_distanceSD->setText(QString::number(data.distance_SD)+" m");
    ui->label_timeExposition->setText(QString::number(data.time_exposition)+" s");
    ui->label_magneticFild->setText(QString::number(data.fild_T)+" T");
    ui->label_temperature->setText(QString::number(data.temp_K)+" K");
    ui->label_wavelenght->setText(QString::number(data.wavelenght)+" A");

    for(int i=0;i<data.n_x;i++) delete [] data.data[i];
    return;
}

void MainWindow::on_pushButton_clicked()
{
    globalDataList.clear();
    for(int i=0;i<dbexplorer->ListData->count();i++){
        if(dbexplorer->ListData->item(i)->isSelected()){
            globalDataList.append(dbexplorer->vDatas.at(i));
        }
    }
    for(int i=0;i<dbexplorer->ListSamples->count();i++){
        if(dbexplorer->ListSamples->item(i)->isSelected()){
            if(i>0) globalSampleID=dbexplorer->vSamples.at(i-1).id;
            else globalSampleID = 0;
            break;
        }
    }

}

/* T-scan */

void MainWindow::on_pushButtonTscanAdd_clicked()
{
    dSelector = new dataselector(&db,&globalDataList,&mainOptions);
    dSelector->show();
    connect(dSelector,SIGNAL(signal_selected(QVector<sDataDB> *)),
            this,SLOT(slot_TscanListAdd(QVector<sDataDB> *)));
}

void MainWindow::slot_TscanListAdd(QVector<sDataDB> *vDataDB){
    sData tmpData;

    disconnect(ui->listWidgetTscan,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_TscanListClick(int)));

    for(int i=0;i<vDataDB->count();i++) vDataTscanDB.append(vDataDB->at(i));

    ui->listWidgetTscan->clear();
    for(int i=0;i<vDataTscanDB.count();i++){
        ui->listWidgetTscan->addItem(vDataTscanDB.at(i).filename+" "
                                     +QString::number(vDataTscanDB.at(i).temp_K)+" K");
        readDataDB(db,vDataTscanDB.at(i),&tmpData,mainOptions);
        vDataTscan.append(tmpData);
    }

    disconnect(dSelector,SIGNAL(signal_selected(QVector<sDataDB> *)),
               this,SLOT(slot_TscanListAdd(QVector<sDataDB> *)));
    delete dSelector;

    connect(ui->listWidgetTscan,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_TscanListClick(int)));
}

void MainWindow::on_pushButtonTscanClr_clicked()
{
    disconnect(ui->listWidgetTscan,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_TscanListClick(int)));
    ui->listWidgetTscan->clear();
    vDataTscan.clear();
    vDataTscanDB.clear();
}

void MainWindow::on_pushButtonTscanMinus_clicked()
{

}

void MainWindow::slot_TscanListClick(int index){
    plotData(plotTscan2D,&vDataTscan[index],mainOptions);
}

void MainWindow::on_pushButtonTscanStart_clicked()
{
    paintSector(plotTscan2D->plot2D,63,63,
                0,90,3,63);
    paintSector(plotTscan2D->plot2D,63,63,
                180,90,3,63);

    QVector<double> vX,vY,vErr;
    QVector<double> vI,vT;
    /*
    average(&vDataTscan[ui->listWidgetTscan->currentRow()],63,63,0,360,3,63,&vX,&vY,&vErr,false,0);
    plotTscan->addCurve(&vX,&vY,true,"red","1");
    */

    vI.clear();
    vT.clear();
    vX.clear();
    vY.clear();
    double S=0;

    for(int i=0;i<vDataTscan.count();i++){
        S=0;
        average(&vDataTscan[i],63,63,0,360,3,63,&vX,&vY,&vErr,false,0);
        for(int j=0;j<vY.count();j++) S+=vY.at(j);
        vX.clear();
        vY.clear();
        vErr.clear();
        vI.append(S);
        vT.append(vDataTscan.at(i).temp_K);
    }

    plotTscan->addCurve(&vT,&vI,true,"red","1");

    plotTscan2D->plot2D->replot();
    plotTscan2D->plot2D->repaint();
    plotTscan2D->plot2D->update();
}

/* End T-scan */


/* Find Ks */
void MainWindow::on_pushButtonKs_add_clicked()
{
    dSelector = new dataselector(&db,&globalDataList,&mainOptions);
    dSelector->show();

    connect(dSelector,SIGNAL(signal_selected(QVector<sDataDB> *)),
            this,SLOT(slot_KsListAdd(QVector<sDataDB>*)));

}

void MainWindow::slot_KsListAdd(QVector<sDataDB> *vDataDB){
    sData tmpData;

    disconnect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_KsListClick(int)));

    for(int i=0;i<vDataDB->count();i++) vDataKsDB.append(vDataDB->at(i));

    ui->listWidget_Ks->clear();
    for(int i=0;i<vDataKsDB.count();i++){
        ui->listWidget_Ks->addItem(vDataKsDB.at(i).filename+" "
                                   +"T="+QString::number(vDataKsDB.at(i).temp_K)+" K "
                                   +"H="+QString::number(vDataKsDB.at(i).fild_T)+" T");
        readDataDB(db,vDataKsDB.at(i),&tmpData,mainOptions);
        vDataKs.append(tmpData);
    }

    disconnect(dSelector,SIGNAL(signal_selected(QVector<sDataDB> *)),
            this,SLOT(slot_KsListAdd(QVector<sDataDB>*)));
    delete dSelector;

    connect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
                this,SLOT(slot_KsListClick(int)));
}

void MainWindow::slot_KsListClick(int index){
    plotData(plotKs2D,&vDataKs[index],mainOptions);
    ui->spinBoxKsInterpolate->setValue(vDataKs.at(index).step_interpolate);

    ui->spinBoxKsCenterX->setValue(vDataKs.at(index).centerX);
    ui->spinBoxKsCenterY->setValue(vDataKs.at(index).centerY);
    ui->spinBoxKsAngle->setValue(vDataKs.at(index).centerAngle);

    plotKs2D->plot2D->clearItems();
    paintCross(plotKs2D->plot2D,mainOptions,&vDataKs[index],
               ui->spinBoxKsCenterX->value(),
               ui->spinBoxKsCenterY->value(),
               ui->spinBoxKsAngle->value());
     plotKs2D->plot2D->replot();
}

void MainWindow::on_pushButton_Ks_minus_clicked()
{
    int index=-1;
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }

    if(index<0) return;

    disconnect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_KsListClick(int)));

    delete ui->listWidget_Ks->item(index);
    vDataKs.remove(index);
    vDataKsDB.remove(index);

    index=-1;
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    if(index>=0) plotData(plotKs2D,&vDataKs[index],mainOptions);

    connect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
                this,SLOT(slot_KsListClick(int)));
}

void MainWindow::on_pushButton_Ks_clr_clicked()
{
    disconnect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_KsListClick(int)));

    ui->listWidget_Ks->clear();
    vDataKs.clear();
    vDataKsDB.clear();

    connect(ui->listWidget_Ks,SIGNAL(currentRowChanged(int)),
                this,SLOT(slot_KsListClick(int)));
}

void MainWindow::on_pushButtonKsInterpolate_clicked()
{
    int index=-1;
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    if(index<0) return;
    interpolate(db,mainOptions,&vDataKsDB[index],&vDataKs[index],ui->spinBoxKsInterpolate->value());
    plotData(plotKs2D,&vDataKs[index],mainOptions);
}

void MainWindow::slot_KsCenterChanged(double value){
    int index = -1;
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    if(index<0) return;
    plotKs2D->plot2D->clearItems();
    paintCross(plotKs2D->plot2D,mainOptions,&vDataKs[index],
               ui->spinBoxKsCenterX->value(),
               ui->spinBoxKsCenterY->value(),
               ui->spinBoxKsAngle->value());
    plotKs2D->plot2D->replot();
}


void MainWindow::on_pushButtonKsCenterOk_clicked()
{
    int index = -1;
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    if(index<0) return;
    vDataKs[index].centerAngle = ui->spinBoxKsAngle->value();
    vDataKs[index].centerX = ui->spinBoxKsCenterX->value();
    vDataKs[index].centerY = ui->spinBoxKsCenterY->value();
}


/* End Find Ks */





