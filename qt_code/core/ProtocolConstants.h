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
// 力传感器标定（与 controller_code/global_config.bas 同步）
// ===================================================================

constexpr int kForceConfigTableBase = 400;
constexpr int kForceScaleOffset     = 9;   // TABLE(409)：Fz 原始值到 N 的比例
constexpr int kForceZeroOffset      = 10;  // TABLE(410)：Fz 空载原始零位

// ===================================================================
// End Tool（末端工具）
// ===================================================================

constexpr int kRevolutionSpeedTable = 450;  // 公转电机目标转速，单位 rpm
constexpr int kRotationSpeedTable   = 451;  // 自转电机目标转速，单位 rpm

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

// 等待缓冲时的状态寄存器轮询间隔 (ms)。
// 所有 ZAux 调用共用一条以太网链路，1ms 轮询会占满链路、饿死状态监控等并发读取，
// 10ms 对轨迹连续性无影响（缓冲有 10 组 × 100 点的余量）
constexpr int kTrajBufferPollIntervalMs = 10;

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
constexpr int kEventRevolutionStart = 11;
constexpr int kEventRevolutionStop  = 12;
constexpr int kEventRotationStart   = 13;
constexpr int kEventRotationStop    = 14;
constexpr int kEventRobotIn     = 21;
constexpr int kEventRobotOut    = 22;
constexpr int kEventStop        = 81;
constexpr int kEventPause       = 82;
constexpr int kEventResume      = 83;
constexpr int kEventErrorReset  = 90;
constexpr int kEventEstop       = 99;

// ===================================================================
// 运动指令ID（与 controller_code/global_config.bas 同步）
// ===================================================================

constexpr int kCmdNone      = 0;
constexpr int kCmdMove      = 1;
constexpr int kCmdMoveAbs   = 2;
constexpr int kCmdMovePt    = 9;
constexpr int kCmdMovePtabs = 10;
constexpr int kCmdMoveDelay = 20;

// CTRL 混合轨迹的段控制指令，记录仍为 7 个 float：
// FORCE_CFG = [30, mode, target_N, contact_N, ramp_N_s, max_adjust_um, reserved]
constexpr int kCmdForceCfg  = 30;
constexpr int kCmdForceOff  = 31;
constexpr int kForceModeSearch = 1;
constexpr int kForceModeHold   = 2;

// ===================================================================
// 手动运动速度（mm/s）
// 指令第 7 字段直接携带速度值，与控制器侧 MAX_MANUAL_SPEED 对应
// ===================================================================

constexpr float kMaxManualSpeedMmPs = 20.0f;

// 末端工具转速绝对值上限（rpm），与控制器侧 MAX_TOOL_SPEED_RPM 对应
constexpr float kMaxToolSpeedRpm = 2000.0f;

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

// ===================================================================
// 机器人状态显示（状态数据 TABLE 环形缓冲）
// ===================================================================

// 控制器每 10 个伺服周期（1kHz 总线 → 100Hz）用一条 TABLE 指令采集一帧 22 通道状态数据
// TABLE[21000..21001] 为 2 个 float64 的 header：[frame_counter, write_index]
// frame_counter 单调递增，0 表示控制器尚未开始采样（总线初始化未完成）
// TABLE[21002+] 为帧区，384 帧 × 22 个 float，帧号 f 的环形位置 = (f-1) % 384
// 100Hz 下覆盖 3.84s，最后使用 TABLE[29449]
constexpr int kStatusTableBase = 21000;
constexpr int kStatusHeaderFloats = 2;
constexpr int kStatusFrameBase = 21002;      // kStatusTableBase + kStatusHeaderFloats
constexpr int kStatusChannelCount = 22;
constexpr int kStatusRingFrameCapacity = 384;
constexpr int kStatusUploadIntervalMs = 100;
