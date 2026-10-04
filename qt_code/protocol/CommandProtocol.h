#pragma once

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 在线命令写入协议 — 封装事件寄存器写入
///
/// 三级事件寄存器 (Level 0/1/2) 对应 MODBUS kRegEventLevel0/1/2。
/// 所有全局控制事件 (Home/Pause/Resume/Stop/Estop/ErrorReset) 通过此类下发。
class CommandProtocol
{
public:
    explicit CommandProtocol(ZMotionDriver* driver);

    // ── 通用事件发送 (指定寄存器级别) ────────────────
    Result sendEventLevel0(int eventId);
    Result sendEventLevel1(int eventId);
    Result sendEventLevel2(int eventId);

    // ── 全局控制事件 ─────────────────────────────────
    Result sendHome();
    Result sendPause();
    Result sendResume();
    Result sendStop();
    Result sendEstop();
    Result sendErrorReset();

    // ── Robot Mode ───────────────────────────────────
    Result sendRobotIn();
    Result sendRobotOut();

private:
    Result sendEvent(int regAddr, int eventId);

    // 事件下发后轮询系统状态寄存器，等待控制器状态机切换到 expected
    Result waitSystemState(uint16_t expected, int errCode, const QString& errMsg);

    ZMotionDriver* driver_ = nullptr;
};
