#ifndef DBEXPLORER_H
#define DBEXPLORER_H

#include <QWidget>
#include <QVector>
#include <QListWidget>
#include <QHBoxLayout>

#include <QSqlQuery>
#include <QSqlDatabase>

#include "structures.h"
#include "config.h"

class DBExplorer : public QWidget
{
    Q_OBJECT
public:
    explicit DBExplorer(QWidget *parent = nullptr);

    QHBoxLayout *mainLayout;
    QListWidget *ListProposals;
    QListWidget *ListSamples;
    QListWidget *ListData;

    QSqlQuery *query;

    QVector<sDataDB> vDatas;
    QVector<sProposalDB> vProposals;
    QVector<sSampleDB> vSamples;

    void getProposals(QVector<sProposalDB> *);
    void getSamples(QVector<sSampleDB> *,int id_proposal=0);
    void getData(QVector<sDataDB> *,int id_sample=0);

    void updateListSamples();
    void updateListProposals();
    void updateListData();

signals:
    void signal_dataSelected(int);
    void signal_clickDataList(int);
public slots:
    void slot_clickDataList(int);
    void slot_connected(QSqlDatabase);
    void slot_clickProposalsList(int);
    void slot_clickSamplesList(int);
    void slot_disconnect();

};

#endif // DBEXPLORER_H
