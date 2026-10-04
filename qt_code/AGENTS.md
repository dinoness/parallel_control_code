# 项目总览

本项目是一个基于 Qt 5.15.2 的 Windows 桌面应用程序，用于通过 ZMotion 运动控制器控制并联机器人。项目名称为 "1202"（对应 1202.pro），采用以太网 MODBUS 协议与控制器通信，支持关节运动、笛卡尔点动、轨迹跟踪和轨迹控制（Ctrl Trace，闭环控制）四种运动模式。

## 技术栈

- **语言标准**: C++11
- **UI 框架**: Qt 5.15.2（Core、Gui、Widgets）
- **构建系统**: qmake（1202.pro）
- **编译器**: MinGW 64-bit
- **第三方库**:
  - ZMotion SDK (zaux.h / zmotion.h) — 正运动控制器底层 C API，提供以太网连接、TABLE 读写、MODBUS 寄存器操作等
  - QXlsx — Excel .xlsx 文件读写
- **运行时依赖**: zaux.dll、zmotion.dll（位于 third_party/zaux/）

## 构建与运行命令

```bash
# 生成 Makefile 并编译（Debug）
qmake 1202.pro -spec win32-g++ "CONFIG+=debug"
mingw32-make

# 或通过 Qt Creator 直接打开 1202.pro 构建

# 清理
mingw32-make clean
```

构建产物输出至 `build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug/`。

运行时需要确保 `zaux.dll` 和 `zmotion.dll` 在可执行文件同目录或 PATH 中。

## 项目结构

```
0610/                          # 主项目目录
├── 1202.pro                   # qmake 工程文件
├── main.cpp                   # 入口：注册 MetaType，初始化 AppContext，启动 MainWindow
├── mainwindow.h / .cpp / .ui  # 主窗口（UI 交互 + 模式管理）
│
├── app/
│   └── AppContext.h / .cpp    # 依赖注入容器，持有并初始化所有 Driver/Protocol/Service
│
├── core/
│   ├── ProtocolConstants.h    # 所有协议常量：MODBUS REG 地址、TABLE 分配、事件 ID、运动指令 ID
│   ├── ControllerInfoTypes.h  # 控制器信息类型：状态快照、传感器 TABLE 配置与批量数据
│   └── Result.h               # 通用结果类型 (ok, code, message)，附带静态工厂方法
│
├── zmotion/
│   └── ZMotionDriver.h / .cpp # 底层驱动：封装 ZAux_* C API（Ethernet 连接、TABLE 读写、
│                              #   MODBUS 寄存器读写、Trigger、RapidStop），线程安全（QMutex）
│
├── protocol/                  # 协议层：每个运动模式一个独立 Protocol 类
│   ├── ControllerProtocol.h / .cpp    # 协议入口占位类（未实质性使用）
│   ├── CommandProtocol.h / .cpp       # 全局控制事件写入（Home/Pause/Resume/Stop/Estop/ErrorReset）
│   ├── JointProtocol.h / .cpp         # Direct Joint 关节指令（固定 TABLE 地址，无缓冲）
│   ├── CartJogProtocol.h / .cpp       # Cart Jog 笛卡尔点动指令（固定 TABLE 地址，无缓冲）
│   ├── TraceProtocol.h / .cpp         # 轨迹模式协议（环形缓冲下发，流式读 .dat 文件；
│   │                                  #   Trace 与 Ctrl Trace 共用，通过 eventId 区分启动事件）
│   └── ControllerInfoProtocol.h / .cpp # 控制器信息读取：系统状态 + 传感器 TABLE 批量读取
│
├── service/                   # 服务层：协调 Protocol 和 Driver
│   ├── ConnectionService.h / .cpp     # 连接管理（IP 扫描、连接/断开），emit connectionChanged 信号
│   ├── MotionService.h / .cpp         # 运动控制协调（Home / Joint / Cart Jog / Trace / 全局控制）
│   ├── TrajectoryService.h / .cpp     # 轨迹文件操作 + 异步轨迹下发管理（在 QThread 中运行 Worker）
│   └── ControllerInfoService.h / .cpp # 控制器信息监控生命周期（状态轮询 + 传感器批量上传）
│
├── motion/
│   ├── TrajectoryTypes.h      # TrajectoryPoint 结构体（7 个 float），.dat 序列化/反序列化
│   └── TrajectoryFile.h / .cpp      # .dat（二进制 LE float）与 .csv（文本）的读写
│
├── worker/
│   ├── TrajectorySendWorker.h / .cpp  # 后台 Worker（QThread），阻塞式轨迹下发，支持取消/暂停
│   └── ControllerInfoWorker.h / .cpp  # 后台 Worker（QThread），双 QTimer 周期读取状态和传感器数据
│
├── third_party/zaux/          # ZMotion SDK 二进制文件
│   ├── zaux.h / zmotion.h     # C API 头文件
│   ├── libzaux.a              # 静态链接库
│   └── zaux.dll / zmotion.dll # 运行时动态库
│
├── QXlsx/                     # QXlsx 库源码（通过 QXlsx.pri 包含）
├── trace_data/                # 轨迹数据文件目录（.dat / .csv，被 .gitignore 忽略）
├── robot_trace.h / .cpp       # 旧的轨迹处理类（直接使用 ZMC_HANDLE，未集成到 AppContext）
└── build/                     # 构建输出目录（被 .gitignore 忽略）
```

## 架构分层

```
┌──────────────────────────────┐
│  UI 层 (MainWindow)          │  用户交互、按钮 / 输入框、状态显示
├──────────────────────────────┤
│  Service 层                  │  业务逻辑：连接管理、运动控制、轨迹文件操作、
│  (ConnectionService,         │  控制器信息监控
│   MotionService,             │
│   TrajectoryService,         │
│   ControllerInfoService)      │
├──────────────────────────────┤
│  Protocol 层                 │  协议封装：状态校验 → TABLE 写入 → MODBUS 通知
│  (JointProtocol,             │
│   CartJogProtocol,           │
│   TraceProtocol,             │
│   CommandProtocol,           │
│   ControllerInfoProtocol)     │
├──────────────────────────────┤
│  Driver 层 (ZMotionDriver)   │  封装 ZAux_* C API，线程安全（QMutex）
├──────────────────────────────┤
│  ZMotion SDK (zaux.dll)      │  正运动控制器 C 库
├──────────────────────────────┤
│  控制器硬件 (以太网)          │  ZMotion 运动控制器
└──────────────────────────────┘
```

每个 Protocol 类在发送指令前都会先读取系统状态寄存器进行前置校验，确认状态满足要求后再写入 TABLE + MODBUS。

## 通信协议

### 指令格式

每条运动指令固定为 7 个 float32（28 字节），通过 TABLE 区域传输：

```
[cmd, param1, param2, param3, param4, param5, ticks]
```

### 资源分配

| 区域 | TABLE 地址 | MODBUS REG | 用途 |
|------|------------|------------|------|
| 关节指令 | 300–334 | 80–89 | Direct Joint，5 条环形缓冲 |
| 笛卡尔点动 | 350–384 | 70–79 | Cart Jog，5 条环形缓冲 |
| 轨迹指令 | 1000–7999 | 50–69 | Trace，10 组 × 100 条 = 1000 条缓冲 |
| 传感器数据 | 8000+ | 120–122 | 传感器 TABLE 环形缓冲（预留，128 帧 × 12 通道） |
| 系统状态 | — | 5 | 系统状态寄存器 (kRegSystemState) |
| 运动模式 | — | 6 | 运动模式寄存器 (kRegMotionMode) |
| 事件 Level 0 | — | 90 | 最高优先级（Estop、ErrorReset） |
| 事件 Level 1 | — | 91 | 中等优先级 |
| 事件 Level 2 | — | 92 | 一般优先级（运动指令等） |

### 数据状态机

```
kDataBlank (3)  →  kDataUpdate (1)  →  控制器消费 →  kDataUsed (2)  →  kDataBlank
```

- 上位机写 TABLE 后置为 kDataUpdate
- 控制器检测到 kDataUpdate 后执行、置为 kDataUsed
- 上位机检测到非 kDataUpdate 后置为 kDataBlank，可重新写入

### 系统状态

```
kSysBoot (0) → kSysBusInit (1) → kSysServoReady (2) → kSysHoming (3) → kSysReady (4) → kSysRobotMode (5) ⇄ kSysRunning (6)
                                                                                              ↕
                                                                                        kSysPaused (17)
kSysError (18)   kSysEstop (19)
```

（状态值以 `core/ProtocolConstants.h` 为准）

## 运动模式工作流

### 1. Direct Joint（关节运动）
1. UI 点击 "Enter" → MotionService::enterJointMode()
2. 输入 J1–J5 目标值 + 速度等级
3. UI 点击 "Send" → MotionService::sendDirectJoint() → JointProtocol::sendJointCommand()
4. JointProtocol: 校验状态 (kSysServoReady 或 kSysReady) → 写 TABLE[300] → 置 kDataUpdate → 写事件 kEventJoint

### 2. Cart Jog（笛卡尔点动）
1. UI 点击 "Enter" → MotionService::enterCartJogMode()
2. 输入命令 ID + X/Y/Z/Phi/Theta 增量 + 速度等级
3. UI 点击 "Send" → MotionService::sendCartJog() → CartJogProtocol::sendCartJogCommand()
4. CartJogProtocol: 校验状态 (仅 kSysReady) → 写 TABLE[350] → 置 kDataUpdate → 写事件 kEventCartJog

### 3. Trace（轨迹跟踪）
1. UI: 进入 Trace 模式 → 点击 "Thread Open" → 点击 "Trace Send"
2. TrajectoryService::startSendTrajectoryAsync()：按文件大小 / 28 字节计算总点数（.dat 为无文件头的纯 float32 流，每点 7 个 float），创建 QThread + TrajectorySendWorker
3. Worker 调用 TraceProtocol::sendTrajectory()：
   - 校验 kSysRobotMode (5)
   - 初始置所有轨迹状态寄存器为 kDataBlank
   - Trigger
   - 预填充前 prefillGroups 组（环形缓冲）
   - 写 kEventTraj 事件 → 控制器开始执行
   - 循环：等待缓冲区可用 → 读文件 → 写 TABLE → 标记 kDataUpdate
   - 等待缓冲区期间（waitBufferReady，1ms 轮询状态寄存器）每 500ms 检查一次系统状态，控制器进入 kSysError / kSysEstop 时中止下发（错误码 3417）
4. 支持暂停（pause，保留下发进度）和取消（cancel）

### 4. Ctrl Trace（轨迹控制，闭环控制）
1. UI: 进入 Ctrl Trace 模式（`MotionMode::CtrlTrace`，同样要求已进入 Robot Mode）→ 点击 "Thread Open"（与 Trace 共用下发线程及 `traceThreadOpened_` 状态）→ 点击 SEND
2. 下发链路与 Trace 完全相同（同一 TABLE 缓冲区 1000–7999、状态寄存器 50–69、TrajectoryService + TrajectorySendWorker 线程管理），仅启动事件不同
3. 实现方式：`TraceProtocol::sendTrajectory()` / `TrajectorySendWorker::startSend()` / `TrajectoryService::startSendTrajectoryAsync()` 均带 `eventId` 参数，Trace 传 `kEventTraj (7)`（默认值），Ctrl Trace 传 `kEventCtrl (9)`
4. 模式进入/退出校验与 Trace 共用 `enterTraceMode()` / `exitTraceMode()` / `canExitTrace()`；暂停/恢复/停止/Robot Mode 退出等 UI 逻辑同样覆盖 CtrlTrace

## 控制器信息监控模块

### 分层

```
MainWindow (UI: label_system_state)
  ↓ 连接信号槽
ControllerInfoService (管理 QThread 生命周期)
  ↓ 创建并管理
ControllerInfoWorker (独立 QThread，双 QTimer)
  ↓ 调用
ControllerInfoProtocol (系统状态读取 + TABLE 批量读取)
  ↓ 调用
ZMotionDriver (getTable / readModbusReg / readModbusRegs)
```

线程管理在 Service 层，定时器逻辑在 Worker 层，协议逻辑在 Protocol 层。

### 状态监控

1. 连接成功后自动启动，周期 200 ms
2. Worker 通过 QTimer 触发 `pollStateOnce()` → Protocol 读取 `kRegSystemState` → 转换为文本 → 信号传递到 UI
3. `label_system_state` 显示：`ServoReady(2)`、`Ready(4)`、`Running(5)` 等
4. 断开连接前 `ControllerInfoService::stopAll()` 停止所有定时器并退出线程
5. 断开后 `label_system_state` 显示 `Disconnected`

### 传感器 TABLE 上传（预留，默认不启动）

控制器端每 1 ms 采样 12 通道传感器数据写入 TABLE[8000+] 环形缓冲（1024 帧）。上位机不做 1 ms 单点读取，而是按 `uploadIntervalMs`（默认 20 ms）批量读取，避免阻塞急停和轨迹下发。

启动方式（需控制器端已配置对应寄存器）：
```cpp
SensorTableConfig config;
config.uploadIntervalMs = 20;
config.maxFramesPerRead = 100;
ctx_->controllerInfoService()->startSensorUpload(config);
```

### 关键约束

- 所有 ZAux API 调用通过 `ZMotionDriver` 的 `QMutex` 串行化
- 批量读取通过 `maxFramesPerRead` 限制单次量，避免阻塞急停和轨迹下发
- `monitorError` 信号仅 `qDebug()` 打印，不弹窗，避免断线时反复弹窗

## 代码风格约定

- **注释语言**: 中文
- **头文件保护**: 统一使用 `#pragma once`
- **命名约定**:
  - 类名：PascalCase（`ZMotionDriver`, `TrajectoryService`）
  - 成员变量：`xxx_` 后缀（`driver_`, `sending_`）
  - 常量：`k` 前缀 + PascalCase（`kTrajCmdSize`, `kRegSystemState`）
  - 事件/状态常量：`k` 前缀（`kEventHome`, `kSysReady`）
- **错误处理**: 使用 `Result` 类型（ok + code + message），不使用异常
- **线程安全**: `ZMotionDriver` 使用 `QMutex` 保护所有 API 调用；Worker 使用 `std::atomic_bool` 做取消/暂停标志
- **信号槽**: 优先使用 Lambda 连接方式，集中在 Init() 中注册
- **文件组织**: 每个模块头文件和源文件在同级目录，include 路径使用相对路径（`../core/`、`../zmotion/`）
- **跨线程 MetaType**: 跨线程信号槽传递的自定义类型，必须在 `main.cpp` 中 `qRegisterMetaType<>()` 注册

## 错误码

Result 中的 code 字段按模块范围分配：
- 1xxx: ZMotionDriver（1001 重复连接、1002 写 REG、1003 读 REG、1004 Trigger、1005 写 TABLE、1006 读 TABLE、1007 批量读 REG、1010 RapidStop）
- 2xxx: TrajectoryFile（2001 写 dat、2002 读 dat、2003 写 csv、2004 读 csv）
- 3xxx: Protocol 层
  - 3101–3106: ControllerInfoProtocol / ControllerInfoService
  - 3200–3220: JointProtocol
  - 3300–3303: TrajectoryService
  - 3402–3420: TraceProtocol
  - 3501–3520: CartJogProtocol
  - 3701–3709: CommandProtocol / MotionService
  - 3801–3813: CommandProtocol / MotionService（Robot Mode，其中 3810–3813 为进出 Robot Mode 的状态校验与回读确认）

## 测试与调试

- 无自动化测试框架；测试通过实际连接控制器进行
- 日志输出使用 `qDebug()` 打印关键路径（指令下发、状态变更、文件操作）
- UI 状态栏显示连接状态、运动模式、轨迹下发进度
- `label_system_state` 实时显示控制器系统状态（200 ms 刷新）

## 安全注意事项

- E-Stop（急停）通过 `sendEstop()` 实现：先调用 `ZAux_Direct_Rapidstop(handle_, 2)` 硬件停止，再写入 Level 0 最高优先级 ESTOP 事件
- 所有 Protocol 命令发送前必须读取并校验系统状态寄存器
- Trace / Ctrl Trace 模式退出前需检查 `kSysRunning` 状态，运行中不允许退出
- `ZMotionDriver` 禁止拷贝（`= delete`），防止多个对象持有同一 `ZMC_HANDLE`
- 轨迹下发使用 `std::atomic_bool` 取消标志实现安全中断；等待缓冲期间还会周期性检查系统状态，控制器 Error/Estop 时自动中止，避免 Worker 无限阻塞
- 断开控制器连接前必须先调用 `ControllerInfoService::stopAll()`，避免 Worker 继续读取已关闭的 handle
