#include "dialogoptions.h"
#include "ui_dialogoptions.h"

DialogOptions::DialogOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogOptions)
{
    ui->setupUi(this);

    ui->comboBox_axies->addItem("In pixiles");
    ui->comboBox_axies->addItem("In impulse 1/nm");
    ui->comboBox_axies->addItem("In impulse 1/A");

}

DialogOptions::~DialogOptions()
{
    delete ui;
}


void DialogOptions::slot_getOptions(sOptions opt){
    ui->lineEditDataBase->setText(opt.database);
    ui->lineEditHostname->setText(opt.hostNameDB);
    ui->lineEditPassword->setText(opt.passwordDB);
    ui->lineEditUserName->setText(opt.userNameDB);
    ui->comboBox_axies->setCurrentIndex(opt.axies2DPlots);

    ui->lineEditWorkDirectory->setText(opt.workDirecotry);
    return;
}

void DialogOptions::on_buttonBox_accepted()
{
    sOptions opt;
    opt.hostNameDB = ui->lineEditHostname->text();
    opt.passwordDB = ui->lineEditPassword->text();
    opt.userNameDB = ui->lineEditUserName->text();
    opt.database = ui->lineEditDataBase->text();
    opt.workDirecotry = ui->lineEditWorkDirectory->text();
    opt.axies2DPlots = ui->comboBox_axies->currentIndex();

    if(!QDir(CONFIG_DIR).exists()) QDir().mkdir(CONFIG_DIR);
    QFile f(CONFIG_FILE);
    f.open(QIODevice::WriteOnly);
    QXmlStreamWriter xml(&f);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("config");
    xml.writeTextElement("hostnameDB", opt.hostNameDB);
    xml.writeTextElement("loginDB", opt.userNameDB);
    xml.writeTextElement("passDB", opt.passwordDB);
    xml.writeTextElement("database", opt.database);
    xml.writeTextElement("workdirectory",opt.workDirecotry);
    xml.writeTextElement("plot_axies",QString::number(opt.axies2DPlots));
    xml.writeEndElement();

    xml.writeEndDocument();

    f.close();

    emit signal_sendOptions(opt);

}

void DialogOptions::on_pushButton_OpenDialog_clicked()
{
    ui->lineEditWorkDirectory->setText(
    QFileDialog::getExistingDirectory(
    this,"set work directory",ui->lineEditWorkDirectory->text(),0));
}
