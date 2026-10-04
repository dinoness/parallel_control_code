#pragma once

#include <QString>
#include <functional>

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 轨迹下发选项
struct TrajectorySendOptions
{
    /// @brief 进度回调：参数(已发送组数, 总组数)
    std::function<void(int, int)> onProgress;

    /// @brief 取消检查：返回 true 表示请求取消
    std::function<bool()> isCancelled;

    /// @brief 暂停检查：返回 true 表示暂停下发（保留进度，可恢复）
    std::function<bool()> isPaused;
};

/// @brief 轨迹模式协议 — 封装 Trace 模式的所有协议操作
///
/// 轨迹下发: 流式读取 .dat 文件，分组写入 TABLE[kTrajTableStart]。
/// 下发前校验系统状态 == kSysRobotMode (5)，先统一置状态寄存器为 kDataBlank 再发事件。
class TraceProtocol
{
public:
    explicit TraceProtocol(ZMotionDriver* driver);

    // ── 模式切换 ──────────────────────────────────────
    /// @brief 进入轨迹模式 — 仅验证状态，不下发事件
    Result enterTraceMode();

    /// @brief 退出轨迹模式 — 重置 sending_ 标志
    Result exitTraceMode();

    /// @brief 检查是否允许退出 — 读取系统状态，SYS_RUNNING 时返回失败
    Result canExitTrace();

    // ── 轨迹下发 ──────────────────────────────────────
    /// @brief 流式下发轨迹 (含 Trigger)
    /// @param datFilePath  .dat 文件完整路径
    /// @param totalPoints  轨迹总点数（用于进度计算）
    /// @param eventId      预填充完成后写入的启动事件 ID，
    ///                     Trace 模式为 kEventTraj，轨迹控制 (Ctrl Trace) 模式为 kEventCtrl
    /// @note 流程: 校验 kSysRobotMode → 置状态寄存器为 kDataBlank → Trigger →
    ///       循环: 等待缓冲区空闲 → 读文件一块 → 写 TABLE → 标记 kDataUpdate →
    ///       写 eventId 事件
    Result sendTrajectory(const QString& datFilePath,
                          int totalPoints,
                          const TrajectorySendOptions& options = TrajectorySendOptions(),
                          int eventId = kEventTraj);


private:
    // ── 轨迹下发内部方法 ──────────────────────────────
    /// @brief 等待指定缓冲组状态为可用 (非 kDataUpdate)，1ms 轮询；
    ///        周期性检查系统状态，控制器 Error/Estop 时退出，取消亦可中断
    Result waitBufferReady(int groupId, const std::function<bool()>& isCancelled);

    ZMotionDriver* driver_;
    bool           sending_ = false;
};
