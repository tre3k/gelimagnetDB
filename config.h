#ifndef CONFIG_H
#define CONFIG_H

#define UNIX
#define DEBUG

#include <QString>
#include <QStandardPaths>

#ifdef DEBUG
#include <QDebug>
#endif

#define SQLTYPE "QMYSQL"

#ifdef UNIX
#define CONFIG_DIR  QDir::homePath()+QString("/.config/gelimagnetDB")
#define CONFIG_FILE QString(CONFIG_DIR)+"/config.xml"
#endif

#endif // CONFIG_H
