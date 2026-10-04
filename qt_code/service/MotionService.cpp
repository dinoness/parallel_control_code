#include "MotionService.h"

MotionService::MotionService(ZMotionDriver* driver,
                             CommandProtocol* commandProtocol,
                             TraceProtocol* traceProtocol,
                             QObject* parent)
    : QObject(parent)
    , driver_(driver)
    , commandProtocol_(commandProtocol)
    , jointProtocol_(driver)
    , cartJogProtocol_(driver)
    , traceProtocol_(traceProtocol)
{
}

// ── 全局控制 ──────────────────────────────────────────

Result MotionService::sendHome()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3501, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendHome();
}

Result MotionService::sendPause()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3701, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendPause();
}

Result MotionService::sendResume()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3703, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendResume();
}

Result MotionService::sendStop()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3705, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendStop();
}

Result MotionService::sendEstop()
{
    if (driver_ == nullptr) {
        return Result::fail(3707, "ZMotionDriver 未初始化");
    }

    if (!driver_->isOpen()) {
        return Result::fail(3708, "控制器未连接");
    }

    // 第一步：直接请求 ZMotion 快速停止运动
    Result ret = driver_->rapidStop(2);
    if (!ret.ok) {
        return ret;
    }

    // 第二步：写入最高优先级 ESTOP 事件，让控制器 RTBasic 状态机进入 ESTOP
    if (commandProtocol_ == nullptr) {
        return Result::fail(3709, "CommandProtocol 未初始化");
    }

    ret = commandProtocol_->sendEstop();
    if (!ret.ok) {
        return Result::fail(
            ret.code,
            QString("RapidStop 已执行，但 ESTOP 事件写入失败：%1").arg(ret.message)
        );
    }

    return Result::success();
}

Result MotionService::sendErrorReset()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3709, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendErrorReset();
}

// ── Joint 模式 ────────────────────────────────────────

Result MotionService::enterJointMode()
{
    return jointProtocol_.enterJointMode();
}

Result MotionService::exitJointMode()
{
    return jointProtocol_.exitJointMode();
}

Result MotionService::sendDirectJoint(float j1, float j2, float j3,
                                      float j4, float j5, int speedLevel)
{
    float cmd[kJointCmdSize];
    cmd[0] = kCmdMove;
    cmd[1] = j1;
    cmd[2] = j2;
    cmd[3] = j3;
    cmd[4] = j4;
    cmd[5] = j5;
    cmd[6] = static_cast<float>(speedLevel);

    return jointProtocol_.sendJointCommand(cmd);
}

// ── Cart Jog 模式 ─────────────────────────────────────

Result MotionService::enterCartJogMode()
{
    return cartJogProtocol_.enterCartJogMode();
}

Result MotionService::exitCartJogMode()
{
    return cartJogProtocol_.exitCartJogMode();
}

Result MotionService::sendCartJog(int cmdId,
                                  float x, float y, float z,
                                  float phi, float theta, int speedLevel)
{
    float cmd[kCartJogCmdSize];
    cmd[0] = static_cast<float>(cmdId);
    cmd[1] = x;
    cmd[2] = y;
    cmd[3] = z;
    cmd[4] = phi;
    cmd[5] = theta;
    cmd[6] = static_cast<float>(speedLevel);

    return cartJogProtocol_.sendCartJogCommand(cmd);
}

// ── Trace 模式 ────────────────────────────────────────

Result MotionService::enterTraceMode()
{
    if (traceProtocol_ == nullptr) {
        return Result::fail(3408, "TraceProtocol 未初始化");
    }

    return traceProtocol_->enterTraceMode();
}

Result MotionService::exitTraceMode()
{
    if (traceProtocol_ == nullptr) {
        return Result::fail(3416, "TraceProtocol 未初始化");
    }

    return traceProtocol_->exitTraceMode();
}

Result MotionService::canExitTraceMode()
{
    if (traceProtocol_ == nullptr) {
        return Result::fail(3409, "TraceProtocol 未初始化");
    }

    return traceProtocol_->canExitTrace();
}

// ── Robot Mode ────────────────────────────────────────

Result MotionService::enterRobotMode()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3801, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendRobotIn();
}

Result MotionService::exitRobotMode()
{
    if (commandProtocol_ == nullptr) {
        return Result::fail(3802, "CommandProtocol 未初始化");
    }
    return commandProtocol_->sendRobotOut();
}
