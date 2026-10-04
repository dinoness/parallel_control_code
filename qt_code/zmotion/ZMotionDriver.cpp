#include "ZMotionDriver.h"
#include <QMetaType>
#include <QByteArray>
#include <QStringList>

ZMotionDriver::ZMotionDriver()
    : handle_(nullptr),
      opened_(false)
{
    qRegisterMetaType<ZMC_HANDLE>("ZMC_HANDLE");
}

ZMotionDriver::~ZMotionDriver()
{
    closeEth();
}

Result ZMotionDriver::openEth(const QString& ip)
{
    QMutexLocker locker(&mutex_);

    if (opened_) {
        return Result::fail(1001, "控制器已经连接，请勿重复连接");
    }

    QByteArray ipBytes = ip.toLocal8Bit();

    ZMC_HANDLE tempHandle = nullptr;

    int ret = ZAux_OpenEth(ipBytes.data(), &tempHandle);

    if (ret != 0 || tempHandle == nullptr) {
        handle_ = nullptr;
        opened_ = false;

        return Result::fail(
            ret,
            QString("以太网连接控制器失败，IP=%1，错误码=%2")
                .arg(ip)
                .arg(ret)
        );
    }

    handle_ = tempHandle;
    opened_ = true;

    return Result::success();
}



Result ZMotionDriver::closeEth()
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        handle_ = nullptr;
        opened_ = false;
        return Result::success();
    }

    int ret = ZAux_Close(handle_);

    handle_ = nullptr;
    opened_ = false;

    if (ret != 0) {
        return Result::fail(
            ret,
            QString("关闭控制器连接失败，错误码=%1").arg(ret)
        );
    }

    return Result::success();
}



bool ZMotionDriver::isOpen() const
{
    QMutexLocker locker(&mutex_);
    return opened_ && handle_ != nullptr;
}


Result ZMotionDriver::ipScan(QStringList& ipList)
{
    ipList.clear();

    const int kBufferSize = 10240;
    QByteArray buffer(kBufferSize, '\0');

    int ret = ZAux_SearchEthlist(buffer.data(), kBufferSize - 1, 200);

    if (ret != ERR_OK) {
        return Result::fail(
            ret,
            QString("IP扫描失败，错误码=%1").arg(ret)
        );
    }

    QString raw(buffer);
    QStringList items = raw.split(' ', Qt::SkipEmptyParts);

    if (items.isEmpty()) {
        return Result::fail(ret, "未搜索到控制器");
    }

    ipList = items;
    return Result::success();
}


Result ZMotionDriver::trigger()
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1004, "控制器未连接，无法 Trigger");
    }

    int ret = ZAux_Trigger(handle_);

    if (ret != 0) {
        return Result::fail(ret,
            QString("Trigger 失败，错误码=%1").arg(ret));
    }

    return Result::success();
}


Result ZMotionDriver::rapidStop(int mode)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1010, "控制器未连接，无法执行 RapidStop");
    }

    int ret = ZAux_Direct_Rapidstop(handle_, mode);

    if (ret != 0) {
        return Result::fail(
            ret,
            QString("ZAux_Direct_Rapidstop 执行失败，mode=%1，错误码=%2")
                .arg(mode)
                .arg(ret)
        );
    }

    return Result::success();
}


Result ZMotionDriver::setTable(int start, int count, const float* data)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1005, "控制器未连接，无法写 TABLE");
    }

    int ret = ZAux_Direct_SetTable(handle_, start, count, const_cast<float*>(data));

    if (ret != 0) {
        return Result::fail(ret,
            QString("写 TABLE(%1, %2) 失败，错误码=%3")
                .arg(start).arg(count).arg(ret));
    }

    return Result::success();
}


Result ZMotionDriver::writeModbusReg(int addr, uint16 value)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1002, "控制器未连接，无法写 MODBUS_REG");
    }

    int ret = ZAux_Modbus_Set4x(handle_, addr, 1, &value);

    if (ret != 0) {
        return Result::fail(
            ret,
            QString("写 MODBUS_REG(%1) 失败，value=%2，错误码=%3")
                .arg(addr)
                .arg(value)
                .arg(ret)
        );
    }

    return Result::success();
}


Result ZMotionDriver::readModbusReg(int addr, uint16& value)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1003, "控制器未连接，无法读 MODBUS_REG");
    }

    int ret = ZAux_Modbus_Get4x(handle_, addr, 1, &value);

    if (ret != 0) {
        return Result::fail(
            ret,
            QString("读 MODBUS_REG(%1) 失败，错误码=%2")
                .arg(addr)
                .arg(ret)
        );
    }

    return Result::success();
}

Result ZMotionDriver::getTable(int start, int count, QVector<float>& data)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1006, "控制器未连接，无法读 TABLE");
    }

    data.resize(count);

    int ret = ZAux_Direct_GetTable(handle_, start, count, data.data());

    if (ret != 0) {
        data.clear();
        return Result::fail(
            ret,
            QString("读 TABLE(%1, %2) 失败，错误码=%3")
                .arg(start).arg(count).arg(ret)
        );
    }

    return Result::success();
}

Result ZMotionDriver::readModbusRegs(int start, int count, QVector<uint16>& values)
{
    QMutexLocker locker(&mutex_);

    if (!opened_ || handle_ == nullptr) {
        return Result::fail(1007, "控制器未连接，无法批量读 MODBUS_REG");
    }

    values.resize(count);

    int ret = ZAux_Modbus_Get4x(handle_, start, count, values.data());

    if (ret != 0) {
        values.clear();
        return Result::fail(
            ret,
            QString("批量读 MODBUS_REG(%1, %2) 失败，错误码=%3")
                .arg(start).arg(count).arg(ret)
        );
    }

    return Result::success();
}
