#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>

#include "../core/ProtocolConstants.h"
#include "../core/ControllerInfoTypes.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 控制器信息读取协议 — 封装系统状态读取、传感器 TABLE 环形缓冲读取
///
/// 控制器状态显示周期为 200 ms，属于低频监控。
/// 传感器信号由控制器端每 1 ms 采样并写入 TABLE 环形缓冲，
/// 上位机不做 1 ms 单点读取，而是按 uploadIntervalMs 批量读取多帧数据。
class ControllerInfoProtocol
{
public:
    explicit ControllerInfoProtocol(ZMotionDriver* driver);

    /// @brief 读取系统状态寄存器并填充快照
    Result readSystemState(ControllerStateSnapshot& snapshot);

    /// @brief 从 TABLE 环形缓冲批量读取传感器数据帧
    /// @param config 传感器配置
    /// @param lastReadIndex 上次读取结束的环形索引，读取后更新
    /// @param lastFrameCounter 上次读取的最后帧计数器，用于覆盖检测
    /// @param batch 输出：批量数据
    Result readSensorBatch(const SensorTableConfig& config,
                           int& lastReadIndex,
                           quint64& lastFrameCounter,
                           SensorTableBatch& batch);

    /// @brief 将系统状态值转换为可读字符串
    static QString systemStateToText(uint16_t state);

private:
    /// @brief 读取连续帧数据（无回绕情况）
    Result readContiguousFrames(const SensorTableConfig& config,
                                int startFrame,
                                int frameCount,
                                quint64 baseFrameCounter,
                                SensorTableBatch& batch);

    ZMotionDriver* driver_ = nullptr;
};
