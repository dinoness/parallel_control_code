#pragma once

#include <QObject>
#include <QString>

#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

class ConnectionService : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionService(ZMotionDriver* driver, QObject* parent = nullptr);

    Result connectToController(const QString& ip);
    Result disconnectFromController();
    Result ipScan(QStringList& ipList);

    bool isConnected() const;

signals:
    void connectionChanged(bool connected);

private:
    ZMotionDriver* driver_;
};