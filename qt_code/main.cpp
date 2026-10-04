#include "mainwindow.h"

#include <QApplication>
#include "app/AppContext.h"
#include "core/ControllerInfoTypes.h"
#include "core/Result.h"
#include "motion/TrajectoryTypes.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<Result>("Result");
    qRegisterMetaType<TrajectoryPoint>("TrajectoryPoint");
    qRegisterMetaType<QVector<TrajectoryPoint>>("QVector<TrajectoryPoint>");
    qRegisterMetaType<ControllerStateSnapshot>("ControllerStateSnapshot");
    qRegisterMetaType<SensorTableConfig>("SensorTableConfig");
    qRegisterMetaType<SensorSampleFrame>("SensorSampleFrame");
    qRegisterMetaType<SensorTableBatch>("SensorTableBatch");

    AppContext appContext;

    MainWindow w(&appContext);
    w.show();
    return a.exec();
}
