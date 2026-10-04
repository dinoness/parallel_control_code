#pragma once

#include <QString>
#include <QVector>
#include <QMutex>

#include "zaux.h"
#include "zmotion.h"

#include "../core/Result.h"



class ZMotionDriver
{
public:
    ZMotionDriver();
    ~ZMotionDriver();

    // 禁止拷贝，避免两个对象持有同一个 ZMC_HANDLE
    ZMotionDriver(const ZMotionDriver&) = delete;
    ZMotionDriver& operator=(const ZMotionDriver&) = delete;


    Result openEth(const QString& ip);
    Result closeEth();

    bool isOpen() const;

    Result ipScan(QStringList& ipList);
    Result trigger();
    Result rapidStop(int mode = 2);
    Result setTable(int start, int count, const float* data);

    /// @brief 批量读取 TABLE，用于传感器数据上传等
    Result getTable(int start, int count, QVector<float>& data);

    Result writeModbusReg(int addr, uint16 value);
    Result readModbusReg(int addr, uint16& value);

    /// @brief 批量读取 MODBUS REG 寄存器
    Result readModbusRegs(int start, int count, QVector<uint16>& values);

private:
    ZMC_HANDLE handle_;
    bool opened_;  // 记录是否已连接

    mutable QMutex mutex_;
};
