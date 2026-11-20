#pragma once

#include <QDateTime>
#include <QVector>
#include <QString>
#include <QMetaType>
#include <stdint.h>

#include "ProtocolConstants.h"

/// @brief 控制器系统状态快照
struct ControllerStateSnapshot
{
    uint16_t systemState = 0;
    QString systemStateText;
    QDateTime timestamp;
};

/// @brief 传感器 TABLE 环形缓冲配置
struct SensorTableConfig
{
    int tableBase        = kSensorTableBase;
    int channelCount     = kSensorChannelCount;
    int ringFrameCapacity = kSensorRingFrameCapacity;

    int writeIndexReg    = kRegSensorWriteIndex;
    int frameCounterReg  = kRegSensorFrameCounter;

    // 上位机批量读取周期 (ms)，不是控制器采样周期
    int uploadIntervalMs = kSensorDefaultUploadIntervalMs;

    // 每次最多读取多少帧，防止一次读取时间太长
    int maxFramesPerRead = 100;
};

/// @brief 单帧传感器采样数据
struct SensorSampleFrame
{
    quint64 frameCounter = 0;
    int ringIndex = 0;
    QDateTime hostTimestamp;
    QVector<float> values;
};

/// @brief 批量传感器数据
struct SensorTableBatch
{
    QVector<SensorSampleFrame> frames;
    bool overflow = false;
    int availableFrames = 0;
    int droppedFrames = 0;
    QDateTime timestamp;
};

/// @brief 单帧机器人状态数据（控制器以 100Hz 写入 TABLE[21002+] 环形缓冲）
///
/// 帧内 16 通道：[0-4] dL支链伸缩量(um) [5-9] 电机扭矩(‰)
/// [10-14] 末端位姿 x,y,z(um),phi,theta(角秒) [15] ee_valid
/// 力信号不进入本帧，只走 29450+ 的 1kHz 环形区（ForceStreamFrame）；
/// 编码器和力控派生状态不上传
struct StatusSampleFrame
{
    quint64 frameCounter = 0;
    bool eeValid = false;
    float dL[5] = {0};        // 支链伸缩量 um
    float torque[5] = {0};    // 电机扭矩 千分比
    float ee[5] = {0};        // 末端位姿 x,y,z(um), phi,theta(角秒)
    QDateTime hostTimestamp;
};

/// @brief 批量机器人状态数据
struct StatusTableBatch
{
    QVector<StatusSampleFrame> frames;
    bool overflow = false;       // 环形区被覆盖
    quint64 droppedFrames = 0;   // 丢帧数
    QDateTime timestamp;
};

/// @brief 单帧高频Fz数据（控制器每1ms写入一帧）
struct ForceStreamFrame
{
    quint64 frameCounter = 0;
    float fzRaw = 0.0f;
    float fzNewton = 0.0f;
    QDateTime hostTimestamp;
};

/// @brief 批量高频Fz数据
struct ForceStreamBatch
{
    QVector<ForceStreamFrame> frames;
    bool overflow = false;
    quint64 droppedFrames = 0;
    QDateTime timestamp;
};

Q_DECLARE_METATYPE(ControllerStateSnapshot)
Q_DECLARE_METATYPE(SensorTableConfig)
Q_DECLARE_METATYPE(SensorSampleFrame)
Q_DECLARE_METATYPE(SensorTableBatch)
Q_DECLARE_METATYPE(StatusSampleFrame)
Q_DECLARE_METATYPE(StatusTableBatch)
Q_DECLARE_METATYPE(ForceStreamFrame)
Q_DECLARE_METATYPE(ForceStreamBatch)
