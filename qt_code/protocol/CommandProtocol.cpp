#include "CommandProtocol.h"
#include <QThread>

CommandProtocol::CommandProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

Result CommandProtocol::sendEvent(int regAddr, int eventId)
{
    if (driver_ == nullptr) {
        return Result::fail(3801, "ZMotionDriver 未初始化");
    }

    if (!driver_->isOpen()) {
        return Result::fail(3802, "控制器未连接");
    }

    return driver_->writeModbusReg(regAddr, static_cast<uint16_t>(eventId));
}

Result CommandProtocol::sendEventLevel0(int eventId)
{
    return sendEvent(kRegEventLevel0, eventId);
}

Result CommandProtocol::sendEventLevel1(int eventId)
{
    return sendEvent(kRegEventLevel1, eventId);
}

Result CommandProtocol::sendEventLevel2(int eventId)
{
    return sendEvent(kRegEventLevel2, eventId);
}

Result CommandProtocol::sendHome()
{
    return sendEventLevel2(kEventHome);
}

Result CommandProtocol::sendPause()
{
    return sendEventLevel2(kEventPause);
}

Result CommandProtocol::sendResume()
{
    return sendEventLevel2(kEventResume);
}

Result CommandProtocol::sendStop()
{
    return sendEventLevel2(kEventStop);
}

Result CommandProtocol::sendEstop()
{
    return sendEventLevel0(kEventEstop);
}

Result CommandProtocol::sendErrorReset()
{
    return sendEventLevel0(kEventErrorReset);
}

Result CommandProtocol::sendRobotIn()
{
    if (driver_ == nullptr) {
        return Result::fail(3801, "ZMotionDriver 未初始化");
    }

    if (!driver_->isOpen()) {
        return Result::fail(3802, "控制器未连接");
    }

    // 1. 前置校验：仅 kSysReady 允许进入 Robot Mode
    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    if (sysState != kSysReady) {
        return Result::fail(3810,
            QString("系统状态不允许进入 Robot Mode (当前=%1, 需要=%2)")
                .arg(sysState).arg(kSysReady));
    }

    // 2. 下发进入 Robot Mode 事件
    ret = sendEventLevel2(kEventRobotIn);
    if (!ret.ok) return ret;

    // 3. 后置校验：确认控制器状态机真正切换到 kSysRobotMode，
    //    避免事件被控制器拒绝后 UI 误认为已进入 Robot Mode
    return waitSystemState(kSysRobotMode, 3811, "控制器未进入 Robot Mode");
}

Result CommandProtocol::sendRobotOut()
{
    if (driver_ == nullptr) {
        return Result::fail(3801, "ZMotionDriver 未初始化");
    }

    if (!driver_->isOpen()) {
        return Result::fail(3802, "控制器未连接");
    }

    // 1. 前置校验：仅 kSysRobotMode 允许退出 Robot Mode
    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    if (sysState != kSysRobotMode) {
        return Result::fail(3812,
            QString("控制器当前不在 Robot Mode (当前状态=%1)").arg(sysState));
    }

    // 2. 下发退出 Robot Mode 事件
    ret = sendEventLevel2(kEventRobotOut);
    if (!ret.ok) return ret;

    // 3. 后置校验：确认控制器状态机真正退回 kSysReady
    return waitSystemState(kSysReady, 3813, "控制器未退出 Robot Mode");
}

Result CommandProtocol::waitSystemState(uint16_t expected, int errCode, const QString& errMsg)
{
    // 控制器 RTBasic 异步消费事件，轮询等待状态切换
    const int pollIntervalMs = 50;
    for (int waited = 0; waited < kRobotModeVerifyTimeoutMs; waited += pollIntervalMs) {
        QThread::msleep(pollIntervalMs);

        uint16_t sysState = 0;
        Result ret = driver_->readModbusReg(kRegSystemState, sysState);
        if (!ret.ok) return ret;

        if (sysState == expected) {
            return Result::success();
        }
    }

    return Result::fail(errCode,
        QString("%1 (等待状态=%2 超时 %3 ms)")
            .arg(errMsg).arg(expected).arg(kRobotModeVerifyTimeoutMs));
}
