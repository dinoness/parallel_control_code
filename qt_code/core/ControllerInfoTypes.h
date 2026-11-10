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
/// 帧内 32 通道：[0-4] dL支链伸缩量(um) [5-9] 电机编码器(脉冲)
/// [10-14] 电机扭矩(‰) [15-19] 末端位姿 x,y,z(um),phi,theta(角秒)
/// [20] ee_valid [21-26] 六轴力/力矩原始值 [27] 滤波法向力(N)
/// [28] 斜坡后的目标力(N) [29] Z修正量(um) [30] 力控模式 [31] 传感器状态
struct StatusSampleFrame
{
    quint64 frameCounter = 0;
    bool eeValid = false;
    float dL[5] = {0};        // 支链伸缩量 um
    float encoder[5] = {0};   // 电机编码器 脉冲
    float torque[5] = {0};    // 电机扭矩 千分比
    float ee[5] = {0};        // 末端位姿 x,y,z(um), phi,theta(角秒)
    float forceRaw[6] = {0};  // Fx,Fy,Fz,Mx,My,Mz 原始值
    float forceNormal = 0.0f; // 滤波后的法向力 N
    float forceTarget = 0.0f; // 经斜坡限制后的目标力 N
    float forceAdjustment = 0.0f; // Z 方向力控修正量 um
    int forceState = 0;       // 0=关闭 1=接触搜索 2=恒力保持 3=故障
    int forceSensorStatus = 0; // 0=禁用 1=有效 2=无效 3=断流 4=过力
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

Q_DECLARE_METATYPE(ControllerStateSnapshot)
Q_DECLARE_METATYPE(SensorTableConfig)
Q_DECLARE_METATYPE(SensorSampleFrame)
Q_DECLARE_METATYPE(SensorTableBatch)
Q_DECLARE_METATYPE(StatusSampleFrame)
Q_DECLARE_METATYPE(StatusTableBatch)
