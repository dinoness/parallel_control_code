#pragma once

#include <stdint.h>

// ===================================================================
// 共享常量
// ===================================================================

// ── 数据状态 ─────────────────────────────────────────
constexpr uint16_t kDataUpdate = 1;  // 已更新，等待控制器消费
constexpr uint16_t kDataUsed   = 2;  // 已使用，控制器已取走
constexpr uint16_t kDataBlank  = 3;  // 空闲，可写入

// ===================================================================
// MODBUS REG寄存器分配
// ===================================================================
constexpr int kRegSystemState         = 5;
constexpr int kRegMotionMode          = 6;
constexpr int kRegActiveTask          = 8;
constexpr int kRegEventLevel0         = 90;
constexpr int kRegEventLevel1         = 91;
constexpr int kRegEventLevel2         = 92;   // 事件寄存器

// ===================================================================
// System Statue
// ===================================================================
constexpr uint16_t kSysBoot = 0;
constexpr uint16_t kSysBusInit = 1;
constexpr uint16_t kSysServoReady = 2;
constexpr uint16_t kSysHoming = 3;
constexpr uint16_t kSysReady  = 4;
constexpr uint16_t kSysRobotMode = 5;
constexpr uint16_t kSysRunning  = 6;
constexpr uint16_t kSysPaused = 17;
constexpr uint16_t kSysError = 18;
constexpr uint16_t kSysEstop = 19;

// Robot Mode 进入/退出事件下发后，等待控制器状态机完成切换的轮询超时 (ms)
constexpr int kRobotModeVerifyTimeoutMs = 1000;

// ===================================================================
// Direct Joint (Manual Joint)
// ===================================================================

constexpr int kJointTableStart  = 300;   // TABLE 起始地址
constexpr int kJointCmdSize     = 7;     // 每条指令 7 个 float
constexpr int kJointBufferSize  = 5;     // 环形缓冲指令条数
constexpr int kJointTableSize   = kJointBufferSize * kJointCmdSize;  // 35
constexpr int kRegJointStatusBase  = 80;    // 指令执行状态REG地址 (80~89)

// ===================================================================
// Cart Jog
// ===================================================================

constexpr int kCartJogTableStart  = 350;   // TABLE 起始地址
constexpr int kCartJogCmdSize     = 7;     // 每条指令 7 个 float
constexpr int kCartJogBufferSize  = 5;     // 环形缓冲指令条数
constexpr int kCartJogTableSize   = kCartJogBufferSize * kCartJogCmdSize;  // 35
constexpr int kRegCartJogStatusBase  = 70;    // 指令执行状态REG地址 (70~79)

// ===================================================================
// Traj
// ===================================================================

constexpr int kTrajTableStart  = 1000;   // TABLE 起始地址
constexpr int kTrajCmdSize     = 7;      // 每条指令 7 个 float
constexpr int kTrajGroupSize   = 100;    // 每组指令数
constexpr int kTrajGroupNum    = 10;     // 环形缓冲组数
constexpr int kTrajBlockSize   = kTrajGroupSize * kTrajCmdSize;  // 700 = 每组 TABLE 大小
constexpr int kRegTrajStatusBase  = 50;     // 指令执行状态REG地址 (50~69)

// 等待缓冲期间检查系统状态寄存器的间隔 (ms)
constexpr int kTrajWaitStateCheckIntervalMs = 500;

// ===================================================================
// Event ID
// ===================================================================

constexpr int kEventIdle        = 0;
constexpr int kEventHome        = 1;
constexpr int kEventJoint       = 3;
constexpr int kEventJointDone   = 4;
constexpr int kEventCartJog     = 5;
constexpr int kEventCartJogDone = 6;
constexpr int kEventTraj        = 7;
constexpr int kEventTrajDone    = 8;
constexpr int kEventCtrl        = 9;
constexpr int kEventCtrlDone    = 10;
constexpr int kEventRobotIn     = 21;
constexpr int kEventRobotOut    = 22;
constexpr int kEventStop        = 80;
constexpr int kEventPause       = 81;
constexpr int kEventResume      = 82;
constexpr int kEventErrorReset  = 90;
constexpr int kEventEstop       = 99;

// ===================================================================
// 运动指令ID
// ===================================================================

constexpr int kCmdNone   = 0;
constexpr int kCmdMove   = 1;
constexpr int kCmdMoveAbs = 2;
constexpr int kCmdMovePtabs = 10;
constexpr int kCmdMoveDelay = 20;

// ===================================================================
// 运动速度等级
// ===================================================================

constexpr int kSpeedLevel1   = 1;
constexpr int kSpeedLevel2   = 2;
constexpr int kSpeedLevel3   = 3;

// ===================================================================
// Controller Info / Sensor TABLE Upload
// ===================================================================

// 控制器状态轮询周期，属于低频监控
constexpr int kControllerStatePollIntervalMs = 200;

// 传感器 TABLE 环形缓冲默认配置，后续可根据控制器程序调整
// 放在 8000 之后，与轨迹 TABLE (1000~7999) 不冲突
constexpr int kSensorTableBase = 8000;
constexpr int kSensorChannelCount = 12;

// 默认预留 1024 帧，每帧 12 个 float
constexpr int kSensorRingFrameCapacity = 1024;
constexpr int kSensorFrameFloatCount = kSensorChannelCount;
constexpr int kSensorTableFloatCount = kSensorRingFrameCapacity * kSensorFrameFloatCount;

// 控制器端用于指示当前写入位置的寄存器，后续需与控制器程序对应
constexpr int kRegSensorWriteIndex = 120;
constexpr int kRegSensorFrameCounter = 121;
constexpr int kRegSensorUploadStatus = 122;

// 上位机默认批量上传周期，不等同于控制器采样周期
// 控制器端可以 1 ms 写一次 TABLE，上位机每 20 ms / 50 ms 批量读一次
constexpr int kSensorDefaultUploadIntervalMs = 20;
