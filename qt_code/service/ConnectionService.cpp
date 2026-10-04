#include "ConnectionService.h"

ConnectionService::ConnectionService(ZMotionDriver* driver, QObject* parent)
    : QObject(parent),
      driver_(driver)
{
}



Result ConnectionService::connectToController(const QString& ip)
{
    if (driver_ == nullptr) {
        return Result::fail(1101, "ZMotionDriver 未初始化");
    }

    Result ret = driver_->openEth(ip);

    if (ret.ok) {
        emit connectionChanged(true);
    }

    return ret;
}



Result ConnectionService::disconnectFromController()
{
    if (driver_ == nullptr) {
        return Result::fail(1102, "ZMotionDriver 未初始化");
    }

    Result ret = driver_->closeEth();

    emit connectionChanged(false);

    return ret;
}



bool ConnectionService::isConnected() const
{
    return driver_ != nullptr && driver_->isOpen();
}


Result ConnectionService::ipScan(QStringList& ipList)
{
    if (driver_ == nullptr) {
        return Result::fail(1103, "ZMotionDriver 未初始化");
    }

    return driver_->ipScan(ipList);
}
