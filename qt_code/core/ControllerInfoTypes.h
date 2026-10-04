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

Q_DECLARE_METATYPE(ControllerStateSnapshot)
Q_DECLARE_METATYPE(SensorTableConfig)
Q_DECLARE_METATYPE(SensorSampleFrame)
Q_DECLARE_METATYPE(SensorTableBatch)
