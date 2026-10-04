#include "CartJogProtocol.h"
#include <QDebug>

CartJogProtocol::CartJogProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

// ===================================================================
// 进入 / 退出
// ===================================================================

Result CartJogProtocol::enterCartJogMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3502, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

Result CartJogProtocol::exitCartJogMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3503, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

// ===================================================================
// 指令下发 (无缓冲，直接覆盖 TABLE[kCartJogTableStart])
// ===================================================================

Result CartJogProtocol::sendCartJogCommand(const float cmd[kCartJogCmdSize])
{
    if (driver_ == nullptr) {
        return Result::fail(3504, "ZMotionDriver 未初始化");
    }

    // 1. 读取系统状态，仅 kSysRobotMode 允许下发
    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    if (sysState != kSysRobotMode) {
        return Result::fail(3520,
            QString("系统状态不允许下发 Cart Jog 指令 (当前=%1, 需要=%2)")
                .arg(sysState).arg(kSysRobotMode));
    }

    // 2. 写入指令数据到固定 TABLE 地址
    ret = driver_->setTable(kCartJogTableStart, kCartJogCmdSize, cmd);
    if (!ret.ok) return ret;

    // 3. 通知控制器有新指令
    ret = driver_->writeModbusReg(kRegCartJogStatusBase, kDataUpdate);
    if (!ret.ok) return ret;

    // 4. 下发 Cart Jog 运动事件到 kRegEventLevel2
    ret = driver_->writeModbusReg(kRegEventLevel2,
                                  static_cast<uint16_t>(kEventCartJog));
    if (!ret.ok) return ret;

    return Result::success();
}
