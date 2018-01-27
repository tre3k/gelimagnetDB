#include "functions.h"

void readDataDB(QSqlDatabase db, sDataDB dataDB, sData *data, sOptions options){
    QSqlQuery *query = new QSqlQuery(db);

    QString prefix;
    char buff[4];
    unsigned int tmp;

    QString string_query = "SELECT * FROM data,prefixes,detectors where data.id_prefix=prefixes.id "
                           "AND id_detectors=detectors.id "
                           "AND data.id="+QString::number(dataDB.id);
    query->exec(string_query);

    while(query->next()){
        prefix=query->value("prefix").toString();
        data->temp_K=query->value("temp_K").toDouble();
        data->fild_T=query->value("fild_T").toDouble();
        data->distance_SD=query->value("distance_SD").toDouble();
        data->time_exposition=query->value("time_exposition").toDouble();
        data->monitor1=query->value("mon1").toInt();
        data->monitor2=query->value("mon2").toInt();
        data->resolution=query->value("resolution_mm").toDouble();
        data->n_x=query->value("n_x").toInt();
        data->n_y=query->value("n_y").toInt();
        data->wavelenght=query->value("wave_lenght").toDouble();
    }

    data->id = dataDB.id;
    data->filename = options.workDirecotry+"/db/"+prefix+"/"+dataDB.filename;
    data->step_interpolate = 0;


    /* Добавить условие для того чтобы брать из БД, если нет записи в БД то делать так: */
    data->centerX = (double)(data->n_x-1)/2;
    data->centerY = (double)(data->n_y-1)/2;
    data->centerAngle = 0.0;

    QFile f(data->filename);
    if(!f.open(QIODevice::ReadOnly)){
        QMessageBox::critical(0,"Error open file!","Can't open file: \'"+data->filename+"\'! =(");
        delete query;
        return;
    }

    data->data = new double * [data->n_x];
    for(int i=0;i<data->n_x;i++) data->data[i] = new double [data->n_y];

    for(int i=0;i<data->n_x;i++){
        for(int j=0;j<data->n_y;j++){
            f.read(buff,4);
            tmp = (unsigned int)(((buff[3]&0xff)<<24)|
                                 ((buff[2]&0xff)<<16)|
                                 ((buff[1]&0xff)<<8)|
                                 (buff[0]&0xff));
            data->data[j][i] = (double)tmp/data->time_exposition/data->monitor1;
        }
    }

    f.close();
    delete query;
    return;
}

void plotData(iCasePlot2D *plot, sData *data, sOptions options){
    double minRangeX,maxRangeX;
    double minRangeY,maxRangeY;
    plot->plot2D->ColorMap->data()->clear();
    plot->plot2D->clearGraphs();
    plot->plot2D->ColorMap->data()->setSize(data->n_x,data->n_y);
    switch(options.axies2DPlots){
    case options.InPixels:
        minRangeX = 0;
        maxRangeX = data->n_x;
        minRangeY = 0;
        maxRangeY = data->n_y;
        plot->plot2D->xAxis->setLabel("x pix");
        plot->plot2D->yAxis->setLabel("y pix");
        break;
    case options.InImpulseNm:
        maxRangeX = 10*data->n_x*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght;
        minRangeX = -maxRangeX;
        maxRangeY = 10*data->n_y*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght;
        minRangeY = -maxRangeY;

        plot->plot2D->xAxis->setLabel("x 1/nm");
        plot->plot2D->yAxis->setLabel("y 1/nm");
        break;
    case options.InImpulseA:
        maxRangeX = data->n_x*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght;
        minRangeX = -maxRangeX;
        maxRangeY = data->n_y*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght;
        minRangeY = -maxRangeY;

        plot->plot2D->xAxis->setLabel("x 1/A");
        plot->plot2D->yAxis->setLabel("y 1/A");
        break;
    }

    maxRangeX /= pow(2,data->step_interpolate);
    minRangeX /= pow(2,data->step_interpolate);
    maxRangeY /= pow(2,data->step_interpolate);
    minRangeY /= pow(2,data->step_interpolate);

    plot->plot2D->ColorMap->data()->setRange(QCPRange(minRangeX,maxRangeX),
                                             QCPRange(minRangeY,maxRangeY));

    for(int xIndex=0;xIndex<data->n_x;xIndex++){
        for(int yIndex=0;yIndex<data->n_y;yIndex++){
            plot->plot2D->ColorMap->data()->setCell(xIndex,yIndex,data->data[xIndex][yIndex]);
        }
    }


    if(!plot->checkBoxManual->isChecked()) plot->plot2D->ColorMap->rescaleDataRange();
    plot->plot2D->ColorMap->rescaleAxes(false);
    plot->plot2D->rescaleAxes(false);
    plot->plot2D->replot();

    return;
}

void average(sData *data,
             double x0, double y0,
             double angle, double oangle,
             double offset, double len,
             QVector<double> *vX,QVector<double> *vY,
             QVector<double> *vErr,bool interp,int step){

    double x,y;
    int oldx=0,oldy=0;
    int ix,iy,n = 0;
    double r = offset,phi;
    double z = 0.0;
    double z2 = 0.0;

    double sigma;

    //double dr = 1;
    double dr = sqrt(2);
    double dphi = 0.1;
    dphi = atan(1.0/len);

    /*
    if(interp){
        dr = pow(2,step);
        dphi = atan(dr/len);
        if(step>1) dphi = atan(pow(2,ui->spinBoxStepInterpolatePricessing->value())/len);
    }
    */

    vX->clear();
    vY->clear();
    vErr->clear();


    //globalProgressBar->setMaximum(((int)len)-1);

    qDebug() << "dphi = " << dphi;
    qDebug() << "dr = " << dr;

    for(r=offset;r<len;r+=dr){
        for(phi=angle-oangle/2;phi<angle+oangle/2;phi+=dphi){
            toCircle(&x,&y,r,phi);

            ix = doubleToInt(x+x0);
            iy = doubleToInt(y+y0);

            if(!(ix==oldx && iy==oldy)){
                if((ix>0 && iy>0) && (ix<data->n_x && iy<data->n_y)){
                    z += data->data[ix][iy];
                    z2 += data->data[ix][iy]*data->data[ix][iy];
                }else{
                    qDebug () << "Out Range!!!";
                }
                /* Test Pixels */
                /*
                if(ui->checkBoxT4PaintPixels->isChecked()){
                    QCPItemEllipse *el = new QCPItemEllipse(plot2DProcessing);
                    el->setBrush(QBrush(QColor(0,255,0,150)));
                    el->setPen(QPen(Qt::NoPen));
                    el->topLeft->setCoords(ix+0.3,iy+0.3);
                    el->bottomRight->setCoords(ix-0.3,iy-0.3);
                }
                */
                n++;

            }
            oldx = ix;
            oldy = iy;
        }
        sigma = sqrt((z*z-z2)/(double)n);
        //qDebug() << "CKO : " << sigma << " err: " << sqrt(z/(double)n);
        vY->append((double)z/n);
        vErr->append(sqrt(abs(z/(double)n)));

        //vErr->append(sqrt(sigma));
        vX->append(r);
        n=0;
        z=0.0;
        z2=0.0;
        //globalProgressBar->setValue((int)r);
    }

    return;
}

int doubleToInt(double val){
    return (int)(val+0.5);
}

void toCircle(double *x, double *y, double r, double phi){
    phi = 2*M_PI*phi/360;
    *x = r*cos(phi);
    *y = r*sin(phi);
    return;
}

void paintSector(iQCustomPlot *plot,
                 double x0, double y0,
                 double angle, double oangle, double offset, double len){

    double x,y;


    QCPItemLine *line1 = new QCPItemLine(plot);
    line1->setPen(QPen(QColor("white"),1,Qt::SolidLine,Qt::SquareCap,Qt::BevelJoin));

    toCircle(&x,&y,offset,angle+oangle/2);
    line1->start->setCoords(x+x0,y+y0);
    toCircle(&x,&y,len,angle+oangle/2);
    line1->end->setCoords(x+x0,y+y0);

    QCPItemLine *line2 = new QCPItemLine(plot);
    line2->setPen(QPen(QColor("white"),1,Qt::SolidLine,Qt::SquareCap,Qt::BevelJoin));
    toCircle(&x,&y,offset,angle-oangle/2);
    line2->start->setCoords(x+x0,y+y0);
    toCircle(&x,&y,len,angle-oangle/2);
    line2->end->setCoords(x+x0,y+y0);


    return;
}


void interpolate(QSqlDatabase db, sOptions options, sDataDB *dataDBin, sData *dataOut,int step){
    int nx_in,nx_out,ny_in,ny_out;
    sData tmpData;
    readDataDB(db,*dataDBin,&tmpData,options);
    for(int i=0;i<dataOut->n_x;i++) delete [] dataOut->data[i];
    delete [] dataOut->data;

    dataOut->n_x = tmpData.n_x*pow(2,step);
    dataOut->n_y = tmpData.n_y*pow(2,step);
    dataOut->step_interpolate = step;
    dataOut->data = new double * [dataOut->n_x];
    for(int i=0;i<dataOut->n_x;i++) dataOut->data[i] = new double [dataOut->n_y];

    /* interpolate */
    double **data_in;
    double **data_out;

    if(step==0){
        copydata(tmpData.data,dataOut->data,dataOut->n_x,dataOut->n_y);
        return;
    }

    nx_in = (int) tmpData.n_x*pow(2,0);
    ny_in = (int) tmpData.n_y*pow(2,0);
    nx_out= (int) tmpData.n_x*pow(2,1);
    ny_out= (int) tmpData.n_y*pow(2,1);

    data_in = new double * [nx_in];
    for(int i=0;i<nx_in;i++) data_in[i] = new double [ny_in];
    data_out = new double * [nx_in];
    for(int i=0;i<nx_in;i++) data_out[i] = new double [ny_in];

    copydata(tmpData.data,data_in,nx_in,ny_in);

    for(int i=0;i<step;i++){
        nx_in = (int) tmpData.n_x*pow(2,i);
        ny_in = (int) tmpData.n_y*pow(2,i);
        nx_out= (int) tmpData.n_x*pow(2,i+1);
        ny_out= (int) tmpData.n_y*pow(2,i+1);

        for(int j=0;j<nx_in;j++) delete [] data_out[j];
        delete [] data_out;
        data_out = new double * [nx_out];
        for(int j=0;j<nx_out;j++) data_out[j] = new double [ny_out];

        interpolate2up(data_in,data_out,nx_in,ny_in);

        if(i<step-1){
            for(int j=0;j<nx_in;j++) delete [] data_in[j];
            delete [] data_in;
            data_in = new double * [nx_out];
            for(int j=0;j<nx_out;j++) data_in[j] = new double [ny_out];

            copydata(data_out,data_in,nx_out,ny_out);
        }
    }

    for(int i=0;i<dataOut->n_x;i++){
        for(int j=0;j<dataOut->n_y;j++) dataOut->data[i][j] = data_out[i][j];
    }

    for(int j=0;j<nx_in;j++) delete [] data_out[j];
    delete [] data_out;
    for(int j=0;j<nx_in;j++) delete [] data_in[j];
    delete [] data_in;
    return;
}

void interpolate2up(double **dataIn, double **dataOut, int size_x, int size_y){
    for(int i=0;i<size_x;i++){
        for(int j=0;j<size_y;j++){
            dataOut[i*2][j*2] = dataIn[i][j];
        }
    }

    for(int i=0;i<2*size_x-2;i+=2){
        for(int j=0;j<2*size_y-2;j+=2){

            dataOut[i+1][j] = 0.5*(dataOut[i][j]+dataOut[i+2][j]);
            dataOut[i+1][j+2] = 0.5*(dataOut[i][j+2]+dataOut[i+2][j+2]);

            dataOut[i][j+1] = 0.5*(dataOut[i][j]+dataOut[i][j+2]);
            dataOut[i+2][j+1] = 0.5*(dataOut[i+2][j]+dataOut[i+2][j+2]);

            dataOut[i+1][j+1] = 0.25*(dataOut[i+1][j]+dataOut[i+1][j+2]+
                                      dataOut[i][j+1]+dataOut[i+2][j+1]);

        }
    }
}

void copydata(double **data_in,double **data_out,int x,int y){
    for(int i=0;i<x;i++){
        for(int j=0;j<y;j++) data_out[i][j] = data_in[i][j];
    }
}

double pix2Impulse(sData *data, double value){
    /* in 1/A */
    return value*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght;
    //return value*atan(1.0e-3*data->resolution/data->distance_SD)*2*M_PI/data->wavelenght/pow(2,data->step_interpolate);
}


void paintSmallCross(iQCustomPlot *plot, double x, double y){
     QCPItemLine *line1 = new QCPItemLine(plot);
     line1->setPen(QPen(QColor("green"),1,Qt::SolidLine,Qt::SquareCap,Qt::BevelJoin));
     line1->start->setCoords(x,y+2);
     line1->end->setCoords(x,y-2);
     QCPItemLine *line2 = new QCPItemLine(plot);
     line2->setPen(QPen(QColor("green"),1,Qt::SolidLine,Qt::SquareCap,Qt::BevelJoin));
     line2->start->setCoords(x+2,y);
     line2->end->setCoords(x-2,y);
}

void paintCross(iQCustomPlot *plot,sOptions options, sData *data, double x, double y, double phi){
    double x1,x2;
    double y1,y2;

    double Mx2,My2;

    double toImpulse = pix2Impulse(data,1.0);

    Mx2 = (double)(data->n_x-1)/2.0/pow(2,data->step_interpolate);
    My2 = (double)(data->n_y-1)/2.0/pow(2,data->step_interpolate);

    switch(options.axies2DPlots){
    case options.InPixels:
        break;

    case options.InImpulseNm:
        //Mx2 *= toImpulse;
        //My2 *= toImpulse;

        x -= (double)(data->n_x-1)/2.0/pow(2,data->step_interpolate);
        y -= (double)(data->n_y-1)/2.0/pow(2,data->step_interpolate);
        //x *= toImpulse;
        //y *= toImpulse;

        x = pix2Impulse(data,x);
        y = pix2Impulse(data,y);

        break;

    case options.InImpulseA:
        x -= (double)(data->n_x-1)/2.0/pow(2,data->step_interpolate);
        y -= (double)(data->n_y-1)/2.0/pow(2,data->step_interpolate);
        x = 10*pix2Impulse(data,x);
        y = 10*pix2Impulse(data,y);
        break;
    }

    //qDebug () << Mx2;

    x1 = My2*tan(2*M_PI*phi/360);
    x2 = Mx2-x1;

    y1 = -Mx2*tan(2*M_PI*phi/360);
    y2 = My2-y1;

    QCPItemLine *line1 = new QCPItemLine(plot);
    line1->setPen(QPen(QColor("black"),1,Qt::DashLine,Qt::SquareCap,Qt::BevelJoin));
    line1->start->setCoords(x1+x,y-My2);
    line1->end->setCoords(x2+x-Mx2,My2+y);

    QCPItemLine *line2 = new QCPItemLine(plot);
    line2->setPen(QPen(QColor("black"),1,Qt::SolidLine,Qt::SquareCap,Qt::BevelJoin));
    line2->start->setCoords(x-Mx2,y1+y);
    line2->end->setCoords(Mx2+x,y2+y-My2);

}

int findIndexInList(QListWidget *listWidget){
    int index = -1;
    for(int i=0;i<listWidget->count();i++){
        if(listWidget->item(i)->isSelected()){
            index=i;
            break;
        }
    }
    return index;
}

void LinearAverage(QVector<double> *vX, QVector<double> *vY, QVector<double> *vErr,
                   sData *data, double input_x0, double input_y0,
                   double input_witdh, double input_offset, int direction){

    int x0,y0,width,offset,k,l;
    int maxRangeX,maxRangeY;

    x0 = doubleToInt(input_x0*pow(2,data->step_interpolate));
    y0 = doubleToInt(input_y0*pow(2,data->step_interpolate));
    width = doubleToInt(input_witdh*pow(2,data->step_interpolate));
    offset = doubleToInt(input_offset*pow(2,data->step_interpolate));

    qDebug () << "x0: " << x0;
    qDebug () << "y0: " << y0;
    qDebug () << "width: " << width;

    maxRangeX = data->n_x;
    maxRangeY = data->n_y;

    vX->clear();
    vY->clear();
    vErr->clear();

    double S=0;
    double err;

    switch(direction){
    case LinearAverageDirection_RIGHT:
        k=offset;
        for(int i=x0+offset;i<maxRangeX;i++){
            S=0;l=0;
            if(i<0 || i>=maxRangeX) break;
            for(int j=y0;j<y0+width;j++){
                if(y0-l<0 || j>=maxRangeY) break;
                //S += data->data[i][j] + data->data[i][j-1];
                S += data->data[i][j] + data->data[i][y0-l];
                l++;
            }
            err = sqrt(S);
            vX->append(k/pow(2,data->step_interpolate));
            vY->append(S/2/l);
            vErr->append(err);
            k++;
        }
        break;

    case LinearAverageDirection_LEFT:
        k=offset;
        for(int i=x0-offset;i>=0;i--){
            S=0;l=0;
            if(i<0 || i>=maxRangeX) break;
            for(int j=y0;j<y0+width;j++){
                if(y0-l<0 || j>=maxRangeY) break;
                S += data->data[i][j] + data->data[i][y0-l];
                l++;
            }
            err = sqrt(S);
            //vX->append(i/pow(2,data->step_interpolate));
            vX->append(k/pow(2,data->step_interpolate));
            vY->append(S/2/l);
            vErr->append(err);
            k++;
        }
        break;

    case LinearAverageDirection_UP:
        k=offset;
        for(int i=y0+offset;i<maxRangeY;i++){
            S=0; l=0;
            if(i<0 || i>=maxRangeY) break;
            for(int j=x0;j<x0+width;j++){
                if(x0-l<0 || j>=maxRangeX) break;
                S += data->data[j][i]+data->data[x0-l][i];
                l++;
            }
            err = sqrt(S);
            vX->append(k/pow(2,data->step_interpolate));
            vY->append(S/2/l);
            vErr->append(err);
            k++;
        }
        break;
    }

    return;
}

sFindMaxHM findMaxHM(QVector<double> *vX, QVector<double> *vY){
    sFindMaxHM ret;
    int tmpIcenter;
    double HM;
    ret.yMax = 0.0;
    ret.background = vY->at(0);
    for(int i=0;i<vY->count();i++){
        if(ret.yMax<vY->at(i)){
            ret.yMax = vY->at(i);
            ret.xMax = vX->at(i);
            tmpIcenter = i;
        }
        if(ret.background > vY->at(i)) ret.background = vY->at(i);
    }

    HM = (ret.yMax-ret.background)/2.0;

    for(int i=tmpIcenter;i>0;i--){
        if((vY->at(i) >= HM)&&(vY->at(i-1) <= HM)){
            ret.xMaxHML = vX->at(i);
            break;
        }
    }
    for(int i=tmpIcenter;i<vY->count()-1;i++){
        if((vY->at(i) >= HM)&&(vY->at(i+1) <= HM)){
            ret.xMaxHMR = vX->at(i);
            break;
        }
    }

    return ret;
}
