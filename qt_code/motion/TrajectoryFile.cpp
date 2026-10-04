#include "TrajectoryFile.h"

#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QTextStream>

TrajectoryFile::TrajectoryFile(const QString& dataDir)
    : dataDir_(dataDir)
{
}

Result TrajectoryFile::writeDat(const QString& fileName, const QVector<TrajectoryPoint>& points)
{
    QString path = QString("%1%2.dat").arg(dataDir_, fileName);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result::fail(2001,
            QString("无法创建文件：%1，错误：%2").arg(path, file.errorString()));
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    for (const TrajectoryPoint& p : points) {
        float arr[kTrajCmdSize];
        p.toArray(arr);
        for (int i = 0; i < kTrajCmdSize; ++i) {
            out << arr[i];
        }
    }

    file.close();
    qDebug() << "writeDat success:" << path << "points:" << points.size();
    return Result::success();
}

Result TrajectoryFile::readDat(const QString& fileName, QVector<TrajectoryPoint>& points)
{
    points.clear();

    QString path = QString("%1%2.dat").arg(dataDir_, fileName);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result::fail(2002,
            QString("无法打开文件：%1，错误：%2").arg(path, file.errorString()));
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);

    while (!in.atEnd()) {
        float arr[kTrajCmdSize];
        bool ok = true;
        for (int i = 0; i < kTrajCmdSize; ++i) {
            in >> arr[i];
            if (in.status() != QDataStream::Ok) {
                ok = false;
                break;
            }
        }
        if (!ok) break;
        points.append(TrajectoryPoint::fromArray(arr));
    }

    file.close();
    qDebug() << "readDat success:" << path << "points:" << points.size();
    return Result::success();
}

Result TrajectoryFile::writeCsv(const QString& fileName, const QVector<TrajectoryPoint>& points)
{
    QString path = QString("%1%2.csv").arg(dataDir_, fileName);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return Result::fail(2003,
            QString("无法创建文件：%1，错误：%2").arg(path, file.errorString()));
    }

    QTextStream out(&file);
    out << "cmd,x,y,z,theta,phi,ticks\n";

    for (const TrajectoryPoint& p : points) {
        out << p.cmd << ','
            << p.x << ','
            << p.y << ','
            << p.z << ','
            << p.theta << ','
            << p.phi << ','
            << p.ticks << '\n';
    }

    file.close();
    qDebug() << "writeCsv success:" << path << "points:" << points.size();
    return Result::success();
}

Result TrajectoryFile::readCsv(const QString& fileName, QVector<TrajectoryPoint>& points)
{
    points.clear();

    QString path = QString("%1%2.csv").arg(dataDir_, fileName);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Result::fail(2004,
            QString("无法打开文件：%1，错误：%2").arg(path, file.errorString()));
    }

    QTextStream in(&file);
    QString header = in.readLine(); // 跳过表头

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(',');
        if (parts.size() < kTrajCmdSize) continue;

        TrajectoryPoint p;
        p.cmd   = parts[0].toFloat();
        p.x     = parts[1].toFloat();
        p.y     = parts[2].toFloat();
        p.z     = parts[3].toFloat();
        p.theta = parts[4].toFloat();
        p.phi   = parts[5].toFloat();
        p.ticks = parts[6].toFloat();

        points.append(p);
    }

    file.close();
    qDebug() << "readCsv success:" << path << "points:" << points.size();
    return Result::success();
}

QString TrajectoryFile::datPath(const QString& fileName) const
{
    return QString("%1%2.dat").arg(dataDir_, fileName);
}
