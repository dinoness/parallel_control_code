#pragma once

#include <QString>
#include <QVector>

#include "TrajectoryTypes.h"
#include "../core/Result.h"

class TrajectoryFile
{
public:
    TrajectoryFile(const QString& dataDir);

    Result writeDat(const QString& fileName, const QVector<TrajectoryPoint>& points);
    Result readDat(const QString& fileName, QVector<TrajectoryPoint>& points);

    Result writeCsv(const QString& fileName, const QVector<TrajectoryPoint>& points);
    Result readCsv(const QString& fileName, QVector<TrajectoryPoint>& points);

    /// @brief 返回 .dat 文件完整路径，例如 "dataDir/01.dat"
    QString datPath(const QString& fileName) const;

private:
    QString dataDir_;
};
