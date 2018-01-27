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

    ui->spinBoxKs1X->setValue(vDataKs.at(index).KsX1);
    ui->spinBoxKs2X->setValue(vDataKs.at(index).KsX2);
    ui->spinBoxKs1Y->setValue(vDataKs.at(index).KsY1);
    ui->spinBoxKs2Y->setValue(vDataKs.at(index).KsY2);

    double KsPix = vDataKs.at(index).KsValue;

    ui->labelKsValue->setText(QString::number(KsPix)+" pix");
    ui->labelKsValueNm->setText(QString::number(10.0*pix2Impulse(&vDataKs[index],KsPix))+" 1/nm");

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
    int index=findIndexInList(ui->listWidget_Ks);
    /*
    for(int i=0;i<ui->listWidget_Ks->count();i++){
        if(ui->listWidget_Ks->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    */
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

void MainWindow::on_pushButtonKsAverage_clicked()
{
    int index=findIndexInList(ui->listWidget_Ks);
    if(index<0) return;

    sFindMaxHM maxHMxR;
    sFindMaxHM maxHMxL;
    sFindMaxHM maxHMyR;
    sFindMaxHM maxHMyL;

    LinearAverage(&vKsAverageXxR,&vKsAverageXyR,&vKsAverageXerrR,&vDataKs[index],
                  ui->spinBoxKsCenterX->value(),
                  ui->spinBoxKsCenterY->value(),
                  ui->spinBoxKsAverageWidthX->value(),
                  ui->spinBoxKsAverage_x_offset->value(),
                  LinearAverageDirection_RIGHT);


    LinearAverage(&vKsAverageXxL,&vKsAverageXyL,&vKsAverageXerrL,&vDataKs[index],
                  ui->spinBoxKsCenterX->value(),
                  ui->spinBoxKsCenterY->value(),
                  ui->spinBoxKsAverageWidthX->value(),
                  ui->spinBoxKsAverage_x_offset->value(),
                  LinearAverageDirection_LEFT);

    maxHMxR = findMaxHM(&vKsAverageXxR,&vKsAverageXyR);
    maxHMxL = findMaxHM(&vKsAverageXxL,&vKsAverageXyL);

    plotKsX->clearItems();plotKsX->clearGraphs();
    QCPItemLine *LineXR = new QCPItemLine(plotKsX);
    QCPItemLine *LineXL = new QCPItemLine(plotKsX);
    //QCPItemText *textXR = new QCPItemText(plotKsX);
    //QCPItemText *textXL = new QCPItemText(plotKsX);

    QCPItemLine *LineHMxR_R = new QCPItemLine(plotKsX);
    QCPItemLine *LineHMxR_L = new QCPItemLine(plotKsX);
    QCPItemLine *LineHMxL_R = new QCPItemLine(plotKsX);
    QCPItemLine *LineHMxL_L = new QCPItemLine(plotKsX);

    LineHMxR_R->start->setCoords(maxHMxR.xMaxHMR,0);
    LineHMxR_R->end->setCoords(maxHMxR.xMaxHMR,maxHMxR.yMax);
    LineHMxR_L->start->setCoords(maxHMxR.xMaxHML,0);
    LineHMxR_L->end->setCoords(maxHMxR.xMaxHML,maxHMxR.yMax);
    LineHMxR_R->setPen(QPen(QColor("red"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineHMxR_L->setPen(QPen(QColor("red"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineXR->start->setCoords(maxHMxR.xMax,0);
    LineXR->end->setCoords(maxHMxR.xMax,maxHMxR.yMax);
    LineXR->setPen(QPen(Qt::red));

    LineHMxL_R->start->setCoords(maxHMxL.xMaxHMR,0);
    LineHMxL_R->end->setCoords(maxHMxL.xMaxHMR,maxHMxL.yMax);
    LineHMxL_L->start->setCoords(maxHMxL.xMaxHML,0);
    LineHMxL_L->end->setCoords(maxHMxL.xMaxHML,maxHMxL.yMax);
    LineHMxL_R->setPen(QPen(QColor("green"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineHMxL_L->setPen(QPen(QColor("green"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineXL->start->setCoords(maxHMxL.xMax,0);
    LineXL->end->setCoords(maxHMxL.xMax,maxHMxL.yMax);
    LineXL->setPen(QPen(Qt::green));

    plotKsX->addCurve(&vKsAverageXxR,&vKsAverageXyR,true,"red","toRight");
    plotKsX->addCurve(&vKsAverageXxL,&vKsAverageXyL,true,"green","toLeft");

    double KsX1,KsX2,KsY1,KsY2;
    KsX2 = ui->spinBoxKsCenterX->value()+maxHMxR.xMaxHML+(maxHMxR.xMaxHMR-maxHMxR.xMaxHML)/2;
    KsX1 = ui->spinBoxKsCenterX->value()-maxHMxL.xMaxHML-(maxHMxL.xMaxHMR-maxHMxL.xMaxHML)/2;
    ui->spinBoxKs1X->setValue(KsX1);
    ui->spinBoxKs2X->setValue(KsX2);


    LinearAverage(&vKsAverageYxL,&vKsAverageYyL,&vKsAverageYerrL,&vDataKs[index],
                  KsX1,
                  0.0,
                  ui->spinBoxKsAverageWidthY->value(),
                  0.0,
                  LinearAverageDirection_UP);
    LinearAverage(&vKsAverageYxR,&vKsAverageYyR,&vKsAverageYerrR,&vDataKs[index],
                  KsX2,
                  0.0,
                  ui->spinBoxKsAverageWidthY->value(),
                  0.0,
                  LinearAverageDirection_UP);

    maxHMyR = findMaxHM(&vKsAverageYxR,&vKsAverageYyR);
    maxHMyL = findMaxHM(&vKsAverageYxL,&vKsAverageYyL);
    plotKsY->clearItems();plotKsY->clearGraphs();

    QCPItemLine *LineYR = new QCPItemLine(plotKsY);
    QCPItemLine *LineYL = new QCPItemLine(plotKsY);
    //QCPItemText *textXR = new QCPItemText(plotKsX);
    //QCPItemText *textXL = new QCPItemText(plotKsX);

    QCPItemLine *LineHMyR_R = new QCPItemLine(plotKsY);
    QCPItemLine *LineHMyR_L = new QCPItemLine(plotKsY);
    QCPItemLine *LineHMyL_R = new QCPItemLine(plotKsY);
    QCPItemLine *LineHMyL_L = new QCPItemLine(plotKsY);

    LineHMyR_R->start->setCoords(maxHMyR.xMaxHMR,0);
    LineHMyR_R->end->setCoords(maxHMyR.xMaxHMR,maxHMyR.yMax);
    LineHMyR_L->start->setCoords(maxHMyR.xMaxHML,0);
    LineHMyR_L->end->setCoords(maxHMyR.xMaxHML,maxHMyR.yMax);
    LineHMyR_R->setPen(QPen(QColor("red"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineHMyR_L->setPen(QPen(QColor("red"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineYR->start->setCoords(maxHMyR.xMax,0);
    LineYR->end->setCoords(maxHMyR.xMax,maxHMyR.yMax);
    LineYR->setPen(QPen(Qt::red));

    LineHMyL_R->start->setCoords(maxHMyL.xMaxHMR,0);
    LineHMyL_R->end->setCoords(maxHMyL.xMaxHMR,maxHMyL.yMax);
    LineHMyL_L->start->setCoords(maxHMyL.xMaxHML,0);
    LineHMyL_L->end->setCoords(maxHMyL.xMaxHML,maxHMyL.yMax);
    LineHMyL_R->setPen(QPen(QColor("green"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineHMyL_L->setPen(QPen(QColor("green"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    LineYL->start->setCoords(maxHMyL.xMax,0);
    LineYL->end->setCoords(maxHMyL.xMax,maxHMyL.yMax);
    LineYL->setPen(QPen(Qt::green));

    plotKsY->addCurve(&vKsAverageYxL,&vKsAverageYyL,true,"green","toLeft");
    plotKsY->addCurve(&vKsAverageYxR,&vKsAverageYyR,true,"red","toRight");

    KsY1 = maxHMyL.xMaxHML + (maxHMyL.xMaxHMR-maxHMyL.xMaxHML)/2.0;
    KsY2 = maxHMyR.xMaxHML + (maxHMyR.xMaxHMR-maxHMyR.xMaxHML)/2.0;

    ui->spinBoxKs1Y->setValue(KsY1);
    ui->spinBoxKs2Y->setValue(KsY2);

    double KsPix =sqrt((KsX1-KsY1)*(KsX1-KsY1) + (KsX2-KsY2)*(KsX2-KsY2))/2.0;

    ui->labelKsValue->setText(QString::number(KsPix)+" pix");
    ui->labelKsValueNm->setText(QString::number(10.0*pix2Impulse(&vDataKs[index],KsPix))+" 1/nm");

    //plotKs2D->p
    paintSmallCross(plotKs2D->plot2D,KsX1,KsY1);
    paintSmallCross(plotKs2D->plot2D,KsX2,KsY2);
    plotKs2D->plot2D->update();
    return;
}

void MainWindow::on_pushButtonKsSetKs_clicked()
{
    int index=findIndexInList(ui->listWidget_Ks);
    vDataKs[index].KsX1 = ui->spinBoxKs1X->value();
    vDataKs[index].KsX2 = ui->spinBoxKs2X->value();
    vDataKs[index].KsY1 = ui->spinBoxKs1Y->value();
    vDataKs[index].KsY2 = ui->spinBoxKs2Y->value();
    vDataKs[index].KsValue = sqrt(
                (vDataKs[index].KsX1-vDataKs[index].KsY1)
               *(vDataKs[index].KsX1-vDataKs[index].KsY1) +
                (vDataKs[index].KsX2-vDataKs[index].KsY2)
               *(vDataKs[index].KsX2-vDataKs[index].KsY2))/2.0;

}

void MainWindow::on_pushButtonFindCenter_clicked()
{
    double KsX1,KsX2,KsY1,KsY2;

    KsX1 = ui->spinBoxKs1X->value();
    KsX2 = ui->spinBoxKs2X->value();
    KsY1 = ui->spinBoxKs1Y->value();
    KsY2 = ui->spinBoxKs2Y->value();

    double centerX,centerY;
    if(KsX1>KsX2){
        centerX = KsX2 + (KsX1-KsX2)/2.0;
    }else{
        centerX = KsX1 + (KsX2-KsX1)/2.0;
    }
    if(KsY1>KsY2){
        centerY = KsY2 + (KsY1-KsY2)/2.0;
    }else{
        centerY = KsY1 + (KsY2-KsY1)/2.0;
    }
    ui->spinBoxKsCenterX->setValue(centerX);
    ui->spinBoxKsCenterY->setValue(centerY);
}

/* End Find Ks */


