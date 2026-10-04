#include "JointProtocol.h"
#include <QDebug>

JointProtocol::JointProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

// ===================================================================
// 进入 / 退出
// ===================================================================

Result JointProtocol::enterJointMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3201, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

Result JointProtocol::exitJointMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3202, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

// ===================================================================
// 指令下发 (无缓冲，直接覆盖 TABLE[kJointTableStart])
// ===================================================================

Result JointProtocol::sendJointCommand(const float cmd[kJointCmdSize])
{
    if (driver_ == nullptr) {
        return Result::fail(3203, "ZMotionDriver 未初始化");
    }

    // 1. 读取系统状态，仅 kSysServoReady 或 kSysReady 允许下发
    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    if (sysState != kSysServoReady && sysState != kSysReady) {
        return Result::fail(3220,
            QString("系统状态不允许下发 Joint 指令 (当前=%1, 需要=%2 或 %3)")
                .arg(sysState).arg(kSysServoReady).arg(kSysReady));
    }

    // 2. 写入指令数据到固定 TABLE 地址
    ret = driver_->setTable(kJointTableStart, kJointCmdSize, cmd);
    if (!ret.ok) return ret;

    // 3. 通知控制器有新指令
    ret = driver_->writeModbusReg(kRegJointStatusBase, kDataUpdate);
    if (!ret.ok) return ret;

    // 4. 下发关节运动事件到 kRegEventLevel2
    ret = driver_->writeModbusReg(kRegEventLevel2,
                                  static_cast<uint16_t>(kEventJoint));
    if (!ret.ok) return ret;

    return Result::success();
}
