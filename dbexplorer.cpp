#include "dbexplorer.h"

DBExplorer::DBExplorer(QWidget *parent) : QWidget(parent)
{
    mainLayout = new QHBoxLayout();

    ListProposals = new QListWidget;
    ListSamples = new QListWidget;
    ListData = new QListWidget;

    mainLayout->addWidget(ListProposals);
    mainLayout->addWidget(ListSamples);
    mainLayout->addWidget(ListData);

    this->setLayout(mainLayout);
}

void DBExplorer::slot_connected(QSqlDatabase db){
    query = new QSqlQuery(db);
    getProposals(&vProposals);
    getSamples(&vSamples);

    ListProposals->clear();
    updateListProposals();
    updateListSamples();

    ListData->setEnabled(true);
    ListProposals->setEnabled(true);
    ListSamples->setEnabled(true);

    ListData->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    connect(ListProposals,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickProposalsList(int)));
    connect(ListSamples,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickSamplesList(int)));

    return;
}

void DBExplorer::getProposals(QVector<sProposalDB> *proposals){
    proposals->clear();

    sProposalDB proposal;

    query->exec("SELECT id,proposal,start_date FROM proposals ORDER BY start_date ASC");
    while(query->next()){
        proposal.id = query->value(0).toInt();
        proposal.name = query->value(1).toString();
        proposal.start_date = query->value(2).toString();
        proposals->append(proposal);
    }

    return;
}

void DBExplorer::getSamples(QVector<sSampleDB> *samples, int id_proposal){
    samples->clear();
    sSampleDB sample;

    if(id_proposal==0) query->exec("SELECT id,sample_name,concentrate,comments FROM samples ");
    else query->exec("SELECT id,sample_name,concentrate,comments FROM samples "
                     "WHERE id_proposal="+QString::number(id_proposal));

    while(query->next()){
        sample.id = query->value(0).toInt();
        sample.sample_name = query->value(1).toString();
        sample.concentrate = query->value(2).toInt();
        sample.comments = query->value(3).toString();
        samples->append(sample);
    }
    return;
}

void DBExplorer::getData(QVector<sDataDB> *data, int id_sample){
    data->clear();
    sDataDB dat;

    QString sQuery = "SELECT id,file_name,id_prefix,id_sample,id_detectors,id_proposal,fild_T,temp_K "
                     "FROM data WHERE";

    if(id_sample==0){
        for(int i=0;i<vSamples.count()-1;i++){
            sQuery += " id_sample="+QString::number(vSamples.at(i).id)+" or";
        }
        sQuery += " id_sample="+QString::number(vSamples.at(vSamples.count()-1).id);
        query->exec(sQuery);
    }else{
        query->exec(sQuery+" id_sample="+QString::number(id_sample));
    }

    while(query->next()){
        dat.id = query->value(0).toInt();
        dat.filename = query->value(1).toString();
        dat.id_prefix = query->value(2).toInt();
        dat.id_sample = query->value(3).toInt();
        dat.id_detector = query->value(4).toInt();
        dat.id_proposal = query->value(5).toInt();
        dat.fild_T = query->value(6).toDouble();
        dat.temp_K = query->value(7).toDouble();
        data->append(dat);
    }

    return;
}

void DBExplorer::updateListProposals(){
    ListProposals->clear();
    ListProposals->addItem("*");
    for(int i=0;i<vProposals.size();i++){
        ListProposals->addItem("("+vProposals.at(i).start_date+") "+vProposals.at(i).name);
    }
    return;
}

void DBExplorer::updateListSamples(){
    ListSamples->clear();
    ListSamples->addItem("*");
    for(int i=0;i<vSamples.size();i++){
        ListSamples->addItem(vSamples.at(i).sample_name+" "
                             +QString::number(vSamples.at(i).concentrate)+"% "
                             +vSamples.at(i).comments);
    }
    return;
}

void DBExplorer::updateListData(){
    for(int i=0;i<vDatas.size();i++){
        ListData->addItem(vDatas.at(i).filename);
    }
}

void DBExplorer::slot_clickProposalsList(int row){
    disconnect(ListSamples,SIGNAL(currentRowChanged(int)),
               this,SLOT(slot_clickSamplesList(int)));
    if(row!=0) getSamples(&vSamples,vProposals.at(row-1).id);
    else getSamples(&vSamples);
    updateListSamples();
    connect(ListSamples,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickSamplesList(int)));
    return;
}

void DBExplorer::slot_clickSamplesList(int row){
    disconnect(ListData,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickDataList(int)));
    ListData->clear();

    if(row!=0) getData(&vDatas,vSamples.at(row-1).id);
    else getData(&vDatas);

    updateListData();
    connect(ListData,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickDataList(int)));
    return;
}

void DBExplorer::slot_disconnect(){

    disconnect(ListProposals,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickProposalsList(int)));
    disconnect(ListSamples,SIGNAL(currentRowChanged(int)),
            this,SLOT(slot_clickSamplesList(int)));

    ListData->setEnabled(false);
    ListProposals->setEnabled(false);
    ListSamples->setEnabled(false);
}

void DBExplorer::slot_clickDataList(int index){
    emit signal_clickDataList(index);
}
