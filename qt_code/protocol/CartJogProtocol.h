#pragma once

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 笛卡尔 Jog 协议 — 封装 Cart Jog 模式的 TABLE 写入与 MODBUS 寄存器操作
///
/// 每次下发覆盖 TABLE[kCartJogTableStart]，无缓冲。
/// 下发前校验系统状态: 仅 kSysRobotMode (5)。
class CartJogProtocol
{
public:
    explicit CartJogProtocol(ZMotionDriver* driver);

    /// @brief 进入 Cart Jog 模式 — 初始化状态，不下发事件
    Result enterCartJogMode();

    /// @brief 退出 Cart Jog 模式 — 清理状态，不下发事件
    Result exitCartJogMode();

    /// @brief 下发一条 Cart Jog 指令到 TABLE[kCartJogTableStart]
    /// @param cmd 7 个 float: [cmd, x, y, z, phi, theta, speed]
    /// @note 先校验 kRegSystemState == kSysReady，
    ///       再写 TABLE、写状态、写事件
    Result sendCartJogCommand(const float cmd[kCartJogCmdSize]);

private:
    ZMotionDriver* driver_;
};
