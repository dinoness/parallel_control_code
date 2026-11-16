#include "ToolProtocol.h"

#include <cmath>

ToolProtocol::ToolProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

Result ToolProtocol::startRevolution(float rpm)
{
    return startTool(kRevolutionSpeedTable, rpm, kEventRevolutionStart, false, 0);
}

Result ToolProtocol::stopRevolution()
{
    return stopTool(kEventRevolutionStop);
}

Result ToolProtocol::startRotation(float rpm, int currentMa)
{
    return startTool(kRotationSpeedTable, rpm, kEventRotationStart, true, currentMa);
}

Result ToolProtocol::stopRotation()
{
    return stopTool(kEventRotationStop);
}

Result ToolProtocol::startTool(int tableAddr, float rpm, int startEvent,
                               bool setCurrent, int currentMa)
{
    if (driver_ == nullptr) {
        return Result::fail(3901, "ZMotionDriver 未初始化");
    }
    if (!driver_->isOpen()) {
        return Result::fail(3902, "控制器未连接");
    }
    if (!std::isfinite(rpm) || rpm == 0.0f || std::fabs(rpm) > kMaxToolSpeedRpm) {
        return Result::fail(3903,
            QString("工具转速需为非零值，且绝对值不超过 %1 rpm")
                .arg(kMaxToolSpeedRpm));
    }
    if (setCurrent && (currentMa <= 0 || currentMa > kMaxRotationCurrentMa)) {
        return Result::fail(3905,
            QString("自转电流需为 1-%1 mA 的整数").arg(kMaxRotationCurrentMa));
    }

    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;
    const bool canStart = (sysState == kSysServoReady || sysState == kSysReady
                           || sysState == kSysRobotMode || sysState == kSysRunning
                           || sysState == kSysPaused);
    if (!canStart) {
        return Result::fail(3904,
            QString("当前系统状态不允许启动末端工具 (当前=%1)").arg(sysState));
    }

    // 自转启动时先下发电流，控制器收到启动事件后写入 0x2300:00
    if (setCurrent) {
        float current = static_cast<float>(currentMa);
        ret = driver_->setTable(kRotationCurrentTable, 1, &current);
        if (!ret.ok) return ret;
    }

    // 保留转速符号，控制器根据正负号控制电机方向
    float speed = rpm;
    ret = driver_->setTable(tableAddr, 1, &speed);
    if (!ret.ok) return ret;

    return driver_->writeModbusReg(kRegEventLevel2,
                                   static_cast<uint16_t>(startEvent));
}

Result ToolProtocol::stopTool(int stopEvent)
{
    if (driver_ == nullptr) {
        return Result::fail(3901, "ZMotionDriver 未初始化");
    }
    if (!driver_->isOpen()) {
        return Result::fail(3902, "控制器未连接");
    }

    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    // 停止不依赖转速输入框，避免非法输入阻止停止操作
    return driver_->writeModbusReg(kRegEventLevel2,
                                   static_cast<uint16_t>(stopEvent));
}
