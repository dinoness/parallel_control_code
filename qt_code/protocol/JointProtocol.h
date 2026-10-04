#pragma once

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 关节指令协议 — 封装 Direct Joint 模式的 TABLE 写入与 MODBUS 寄存器操作
///
/// 每次下发覆盖 TABLE[kJointTableStart]，无缓冲。
/// 下发前校验系统状态: kSysServoReady 或 kSysReady。
class JointProtocol
{
public:
    explicit JointProtocol(ZMotionDriver* driver);

    /// @brief 进入关节模式 — 初始化状态，不下发事件
    Result enterJointMode();

    /// @brief 退出关节模式 — 清理状态，不下发事件
    Result exitJointMode();

    /// @brief 下发一条关节指令到 TABLE[kJointTableStart]
    /// @param cmd 7 个 float: [cmd, j1, j2, j3, j4, j5, speed]
    /// @note 先校验 kRegSystemState ∈ {kSysServoReady, kSysReady}，
    ///       再写 TABLE、写状态、写事件
    Result sendJointCommand(const float cmd[kJointCmdSize]);

private:
    ZMotionDriver* driver_;
};
