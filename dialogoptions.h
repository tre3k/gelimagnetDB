#ifndef DIALOGOPTIONS_H
#define DIALOGOPTIONS_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QLineEdit>

#include <QComboBox>

#include <QDir>

#include <QXmlStreamWriter>
#include <QFile>
#include <QFileDialog>

#include "structures.h"
#include "config.h"

namespace Ui {
class DialogOptions;
}

class DialogOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOptions(QWidget *parent = 0);
    ~DialogOptions();

private:
    Ui::DialogOptions *ui;

private slots:
    void slot_getOptions(sOptions);
    void on_buttonBox_accepted();

    void on_pushButton_OpenDialog_clicked();

signals:
    void signal_sendOptions(sOptions);

};

#endif // DIALOGOPTIONS_H
