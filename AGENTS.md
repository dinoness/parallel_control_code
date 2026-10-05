# AGENTS.md

## 项目概述

本仓库为 **5 轴并联机器人控制系统**，由上下位机两部分组成，两部分分别位于两个独立子目录：

| 子目录 | 角色 | 说明 |
|--------|------|------|
| `controller_code/` | 下位机（控制器固件） | 运行在 ZMC（ZMotion，正运动）运动控制器上的 RTBasic 程序 + 自定义 C 运动学库，通过 EtherCAT 控制 5 个伺服驱动器 |
| `qt_code/` | 上位机（HMI） | Qt 5.15.2 / C++11 Windows 桌面程序（工程名 "1202"），通过以太网 MODBUS 协议与控制器通信，下发指令并监控状态 |
| `refrence/` | 参考目录 | 当前为空 |

**各子目录均有自己的 `AGENTS.md`**（`controller_code/AGENTS.md`、`qt_code/AGENTS.md`），记录了对应子项目的详细架构、构建方法、代码约定和开发状态。修改某一侧代码前必须先读对应文件；本文件只做全局概览和跨侧约定的说明。

## 技术栈

| 部分 | 技术 | 说明 |
|------|------|------|
| 控制器主程序 | RTBasic (.bas) | ZMC 控制器的 BASIC 方言，在 ZDevelop IDE 中编写并直接下载运行 |
| 运动学库 | C (.c/.h) → .so | 自定义并联机器人正逆解（闭环矢量法），交叉编译为 .so，由控制器作为 CFRAME 1000 加载 |
| 上位机 | C++11 / Qt 5.15.2 / qmake | MinGW 64-bit 编译，使用 ZMotion SDK（zaux/zmotion C API）和 QXlsx |
| 上下位机通信 | MODBUS REG + TABLE | 以太网 MODBUS；事件经 MODBUS_REG 下发，数据经 TABLE 区传输 |

## 构建与部署

### 控制器侧（`controller_code/`）

- `.bas` 文件：用 **ZDevelop IDE** 打开工程文件 `v2_1.zpj`，直接下载到控制器运行，无需编译。`main.bas` 为自动运行文件（AutoRun=0），其余 .bas 按 `Down` 编号下载。
- `.c` 运动学库（`frame1000.c` + `myeigen.c` + `myeigen.h`，依赖 `zmcbuildin.h`）：需在 Linux 下用 ZMC 交叉编译工具链编译为 .so，BASIC 端以 `DEFINE_CFRAME`（frame=1000）加载。修改后必须重新编译并更新控制器中的 .so。
- `v2_1.so` 为已有编译产物；`*.so`、`*.zpj`、`*.pdf` 被根目录 `.gitignore` 忽略。

### 上位机侧（`qt_code/`）

```bash
qmake 1202.pro -spec win32-g++ "CONFIG+=debug"
mingw32-make
```

或用 Qt Creator 直接打开 `1202.pro` 构建。产物输出到 `build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug/`（被 .gitignore 忽略）。运行时需 `zaux.dll` 和 `zmotion.dll`（位于 `third_party/zaux/`）在可执行文件同目录或 PATH 中。

## 代码组织

### 控制器侧（`controller_code/`）

- `main.bas` — 主入口：初始化、主循环、事件分发
- `global_config.bas` — 全局常量（寄存器分配、状态枚举、运动参数）
- `fsm.bas` — 有限状态机：事件获取/分发、各状态处理函数、任务桩函数
- `ethercat_mgr.bas` — EtherCAT 总线初始化（扫描、轴映射、启动）
- `home_mgr.bas` / `manual_move_mgr.bas` / `traj_move_mgr.bas` / `ctrL_mgr.bas` — 回零 / 单轴手动 / 轨迹执行 / 力控闭环微调（周期中断任务）
- `robo_config.bas` — 机器人几何参数（动静平台坐标、初始支链长度）
- `safety_mgr.bas` — 安全监控（心跳检测，框架代码）
- `frame1000.c` / `myeigen.c` / `myeigen.h` / `zmcbuildin.h` — CFRAME 运动学库及 ZMC SDK 头文件
- `register_assignment.md` — TABLE 和 MODBUS_REG 资源分配文档（两侧共同遵守的通信契约）
- `RTBasic编程手册V1.1.2.pdf` — ZMC 编程手册

### 上位机侧（`qt_code/`）

分层架构：`MainWindow (UI)` → `service/`（连接、运动、轨迹、信息监控）→ `protocol/`（每种运动模式一个 Protocol 类，发送前做状态校验）→ `zmotion/ZMotionDriver`（封装 ZAux_* C API，QMutex 线程安全）→ `zaux.dll` → 控制器。

- `core/ProtocolConstants.h` — 所有协议常量（MODBUS REG 地址、TABLE 分配、事件 ID），**与控制器侧 `global_config.bas` / `register_assignment.md` 对应，修改必须两侧同步**
- `app/AppContext` — 依赖注入容器
- `motion/` — 轨迹点结构（7 个 float32）与 .dat/.csv 文件读写
- `worker/` — QThread 后台 Worker（轨迹下发、状态/传感器轮询）

## 跨侧通信约定（重要）

上下位机共享同一套通信契约，**任何一侧修改地址或事件定义，另一侧必须同步**：

- 指令格式：每条运动指令固定 7 个 float32 —— `[cmd, p1..p5, ticks]`，经 TABLE 区传输
- TABLE 分配：0–299 结构参数 / 300–349 单轴指令 / 350–399 点动指令 / 1000–7999 轨迹数据 / 8000+ 传感器数据（预留）
- MODBUS_REG 分配：0–49 系统状态 / 50–69 轨迹状态 / 70–79 点动 / 80–89 单轴 / 90–99 事件队列（90 紧急、92 一般命令）
- 数据状态机：`kDataBlank(3) → kDataUpdate(1) → 控制器消费 → kDataUsed(2) → kDataBlank`
- 事件 ID：HOME=1, JOINT=3, CART_JOG=5, TRAJ=7, CTRL=9, STOP=81 等（定义见 `controller_code/global_config.bas` 与 `qt_code/core/ProtocolConstants.h`）
- 权威文档：`controller_code/register_assignment.md` 及两个子目录的 AGENTS.md

## 开发约定

- **注释与日志语言：中文**；标识符为英文
- 控制器侧：常量 `ALL_CAPS`、变量 `snake_case`、函数 `PascalCase`、任务函数 `UPPER_SNAKE`
- 上位机侧：类 `PascalCase`、成员变量 `xxx_` 后缀、常量 `k` 前缀；错误处理用 `Result` 类型（不用异常）；头文件用 `#pragma once`；跨线程信号传递的自定义类型需在 `main.cpp` 注册 `qRegisterMetaType`
- 新增 FSM 状态：在 `global_config.bas` 加常量 → `SMF_DISPATCH` 加分支 → 实现 `Handle_SYS_*()` → 覆盖该状态所有合法事件

## 测试

- **无自动化测试框架**。控制器侧通过 ZDevelop 下载运行、观察 `PRINT` 输出和 MODBUS 状态寄存器验证 FSM 逻辑；桩任务用 `DELAY` 模拟
- 上位机侧通过实际连接控制器联调，`qDebug()` 打印关键路径，UI 状态栏和 `label_system_state`（200ms 轮询）显示运行状态
- 回零、限位等测试需连接实际驱动器硬件

## 安全注意事项

- 急停（ESTOP）为最高优先级事件：上位机先调 `ZAux_Direct_Rapidstop` 硬件停止再写 Level 0 事件；控制器侧任何状态均可触发 ESTOP（控制器侧 `enter_estop()/reset_estop()` 实现尚待完善）
- 所有上位机 Protocol 命令发送前必须读取并校验系统状态寄存器
- 笛卡尔空间运动（CART_JOG、TRAJ、CTRL）必须先完成回零（控制器正解依赖回零）
- 编码器 `ENCODER_PER_ROE = 2^23`，丝杠导程 5mm，注意 `UNITS` 与驱动器电子齿轮比不能重复缩放
- 轨迹下发的 Worker 线程在等待缓冲期间会周期检查系统状态，控制器进入 Error/Estop 时自动中止，避免无限阻塞
