#ifndef ROBOT_TRACE_H
#define ROBOT_TRACE_H


#include <math.h>
#include <QThread>
#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <QDebug>
#include <QDir>
#include "zaux.h"
#include "zmotion.h"
#include "xlsxdocument.h"

class robot_trace:public QObject
{
    Q_OBJECT
public:
    robot_trace();

private:
    QString data_dir;
    QXlsx::Document* p_xlsx = nullptr;
    bool xlsx_init(const QString& cus_file_name);
    bool xlsx_close();

public slots:
    bool trace_to_dat_test(const QString& cus_file_name);
    bool trace_to_controller(ZMC_HANDLE g_handle, const QString& cus_file_name);
    bool dat_to_xlsx(const QString& dat_file_name, const QString& xlsx_file_name);
    bool xlsx_to_dat(const QString& dat_file_name, const QString& xlsx_file_name);

signals:

};

#endif // ROBOT_TRACE_H
