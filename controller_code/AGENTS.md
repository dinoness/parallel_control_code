# AGENTS.md

## 项目概述

本项目为**并联机器人运动控制器软件**，运行在**ZMC（ZMotion Controller）运动控制器**硬件平台上。控制系统通过 EtherCAT 总线连接 5 个伺服驱动器，实现 5 轴并联机器人的运动控制。

- **控制器平台**: ZMC 系列运动控制器（深圳正运动技术）
- **通讯协议**: EtherCAT（与驱动器通讯）、MODBUS REG（与上位机 Qt 程序通讯）
- **机器人类型**: 5 轴并联机器人（非串联），采用闭环矢量法进行运动学解算
- **上位机**: Qt 程序通过 MODBUS 协议下发指令、监控状态

## 技术栈

| 组件 | 语言/格式 | 说明 |
|------|----------|------|
| 主控程序 | RTBasic (.bas) | ZMC 控制器的 BASIC 方言，在 ZDevelop IDE 中编写 |
| 运动学库 | C (.c/.h) | 自定义正逆解算法，编译为 .so 由控制器加载为 CFRAME |
| 向量数学库 | C (.c/.h) | 自实现 3D 向量/矩阵运算（vec3、mat3x3），替代 Eigen |
| SDK 头文件 | C (.h) | `zmcbuildin.h` — ZMC 官方 SDK，提供类型定义和标准数学函数声明 |
| 项目文件 | .zpj | ZDevelop IDE 工程文件 |

## 构建与部署

### .bas 文件
BASIC 源文件通过 **ZDevelop IDE** 直接下载到控制器运行，无需编译。

`v2.zpj` 为 ZDevelop 项目文件，其中 `[FileList]` 节定义了下载到控制器的文件列表及下载编号（`Down` 字段）。当前项目包含 `ecat_mgr.bas`、`global_def.bas`、`main.bas` 三个下载文件，`main.bas` 为自动运行文件。

### .c 文件（运动学库）
C 文件需编译为 .so 共享库，由控制器运行时加载：

1. 在 Linux 环境下使用 ZMC 提供的交叉编译工具链编译
2. 将生成的 .so 文件放到控制器可访问的路径
3. BASIC 代码中通过 `DEFINE_CFRAME` 指令加载（当前使用 frame=1000）

项目中已有编译产物 `t1.so`（位于父目录）作为参考。

### 关键编译产物
- `frame1000.c` + `myeigen.c` + `myeigen.h` → 编译为 .so → 控制器 CFRAME 1000
- `zmcbuildin.h` — ZMC SDK 头文件，提供 `TYPE_FRAME`、`TYPE_TABLE` 等类型及 `sin`/`cos`/`sqrt` 等数学函数声明

## 代码组织结构

```
v2/
├── main.bas              # 主入口：初始化、主循环、事件分发
├── global_config.bas     # 全局常量定义（寄存器分配、状态枚举、运动参数配置）
├── fsm.bas               # 有限状态机：事件获取、分发、状态处理函数、任务桩函数
├── ethercat_mgr.bas      # EtherCAT 总线初始化（扫描、轴映射、启动）
├── home_mgr.bas          # 回零管理（home_robot 函数，5 轴分两组回零）
├── manual_joint_mgr.bas  # 单轴手动控制（通过 MODBUS 接收指令）
├── safety_mgr.bas        # 安全监控（心跳检测，独立任务，待完善）
├── para_config.bas       # 机器人几何参数配置（动静平台坐标、初始支链长度）
├── frame1000.c           # 自定义 CFRAME 1000 正逆解实现（闭环矢量法）
├── myeigen.c             # 自实现 3D 向量/矩阵运算库
├── myeigen.h             # 向量/矩阵运算库头文件
├── zmcbuildin.h          # ZMC 官方 SDK 头文件
├── register_assignment.md # TABLE 和 MODBUS_REG 资源分配文档
└── v2.zpj                # ZDevelop 项目文件
```

### 模块依赖关系

```
main.bas
  ├── global_config.bas    (GLOBAL_DEF, AXIS_CONGIF)
  ├── ethercat_mgr.bas     (Ecat_Init)
  ├── para_config.bas      (rob_para_config1)
  ├── fsm.bas              (CHECK_EVENT, SMF_DISPATCH, 及各 Handle_* 和任务函数)
  │     ├── home_mgr.bas        (home_robot — 通过 RUNTASK 调用)
  │     ├── manual_joint_mgr.bas (MANNUAL_JOINT — 通过 RUNTASK 调用)
  │     ├── traj_move_mgr.bas   (TRAJ_MOVE — 通过 RUNTASK 调用)
  │     └── [待实现] cart_jog 模块
  └── safety_mgr.bas       (独立 WHILE 1 任务)
```

CFRAME 1000 运动学库链：
```
frame1000.c
  ├── myeigen.c / myeigen.h   (向量/矩阵运算)
  └── zmcbuildin.h            (ZMC SDK 类型与函数)
```

## 架构设计

### 有限状态机（FSM）

系统状态转换流程：

```
SYS_BOOT → SYS_BUS_INIT → SYS_SERVO_READY ⇄ SYS_HOMING → SYS_READY ⇄ SYS_RUNNING ⇄ SYS_PAUSED
                               ↑                    ↑                      ↑
                               └── SYS_ERROR ←──────┴── SYS_ESTOP ←────────┘
```

9 种系统状态：
- `SYS_BOOT` (0): 启动
- `SYS_BUS_INIT` (1): 总线初始化中
- `SYS_SERVO_READY` (2): 伺服就绪，未回零
- `SYS_HOMING` (3): 回零中
- `SYS_READY` (4): 已回零，可运动
- `SYS_RUNNING` (5): 运动中
- `SYS_PAUSED` (6): 已暂停
- `SYS_ERROR` (8): 错误
- `SYS_ESTOP` (9): 急停

每种状态有对应的 `Handle_SYS_*()` 处理函数，仅接收该状态下合法的事件。

### 事件驱动机制

上位机通过 MODBUS 寄存器写入事件 ID，控制器在主循环中按优先级扫描（地址越小优先级越高）：

| 优先级 | 寄存器地址 | 级别 |
|--------|-----------|------|
| 紧急 | REG_EVENT_BEGIN+0 (90) | STOP、ESTOP |
| 中等 | REG_EVENT_BEGIN+1 (91) | 运动完成通知（由控制器任务写入） |
| 一般 | REG_EVENT_BEGIN+2 (92) | 上位机命令（HOME、JOINT、CART_JOG、TRAJ 等） |

事件 ID 定义见 `global_config.bas:45-59`（EVENT_HOME=1, EVENT_JOINT=3, EVENT_CART_JOG=5, EVENT_TRAJ=7, EVENT_CTRL=9, EVENT_STOP=81 等）。

### 任务管理

长耗时运动任务通过 `RUNTASK` 启动为后台任务，与主循环并行：

| 任务 ID | 任务名 | 对应函数 | 运动模式 |
|---------|--------|---------|---------|
| TASK_HOEM (1) | 回零 | HOME_TASK() → home_robot() | MODE_HOME |
| TASK_JOINT (2) | 单轴手动 | JOINT_TASK() → MANNUAL_JOINT() | MODE_JOINT_MANUAL |
| TASK_CATR_JOG (3) | 笛卡尔点动 | CART_JOG_TASK()（待实现） | MODE_CART_JOG |
| TASK_TRAJ (4) | 轨迹执行 | TRAJ_TASK() → TRAJ_MOVE() | MODE_TRAJECTORY |
| TASK_CTRL_MOVE (5) | 闭环控制（力控微调） | CTRL_MOVE() → INT_CYCLE 周期执行 PID_MOVE() | MODE_SENSOR_CLOSED |

任务之间互斥，同一时间只能有一个运动任务在运行。暂停/恢复通过 `PAUSETASK`/`RESUMETASK` 实现。
注意：闭环控制不是 RUNTASK 任务，而是 `INT_CYCLE` 中断周期任务（每个 SERVO_PERIOD 执行一次 PID_MOVE），
由 `CTRL_MOVE()` 启动、`CTRL_MOVE_END()` 停止；周期任务读到 cmd_id=0（CMD_NONE）时自行停止并写
`EVENT_CTRL_DONE` 通知 FSM 退出闭环模式。

### 运动学系统

使用 ZMC 自定义 CFRAME（Custom Frame）机制，frame ID=1000。文件 `frame1000.c` 实现了三个入口函数：

- `SOFRAME_INIT1000`: 参数初始化，从 TABLE 读取几何参数
- `SOFRAME_RETRANS1000`（逆解）: 世界坐标(um+角度) → 5 关节脉冲数，使用闭环矢量法
- `SOFRAME_TRANS1000`（正解）: 5 关节脉冲数 → 世界坐标（当前为桩实现，需先回零）

几何参数包括：静平台铰链坐标 B[5]、动平台铰链坐标 M[5]、初始支链长度 limb0[5]。

### 资源分配

详见 `register_assignment.md`。

- **TABLE**: 0-299 结构参数 / 300-349 单轴指令 / 350-399 点动指令 / 400-999 预留 / 1000-9999 轨迹数据
- **MODBUS_REG**: 0-49 系统状态 / 50-69 轨迹状态 / 70-79 点动状态 / 80-89 单轴状态 / 90-99 事件序列

## 开发约定

### 命名规范
- 全局常量：`ALL_CAPS` 风格（如 `SYS_READY`、`REG_SYSTEM_STATE`、`EVENT_HOME`）
- 全局变量：`snake_case` 风格（如 `system_state`、`motion_mode`、`bus_initstate`）
- 函数名：`PascalCase` 风格（如 `Ecat_Init`、`SMF_DISPATCH`、`Handle_SYS_READY`）
- 任务函数名：`UPPER_SNAKE` 风格（如 `HOME_TASK`、`JOINT_TASK`）

### 注释语言
代码注释和 `PRINT` 输出均为**中文**，变量名和关键字为英文。

### 状态机模式
新增状态时需：
1. 在 `global_config.bas` 中添加状态常量和事件常量
2. 在 `SMF_DISPATCH` 中添加对应的 `ELSEIF` 分支
3. 实现对应的 `Handle_SYS_*()` 处理函数
4. 确保每个状态下所有合法事件都有明确处理（包括拒绝无意义事件）

### MODBUS 寄存器使用
- 使用 `MODBUS_REG(地址)` 读写，存储 16 位 INT
- 寄存器分配需更新 `register_assignment.md`
- 上位机写入命令前应检查 CMD_STATE 是否空闲

### 运动学修改
修改 `frame1000.c` 后需重新编译 .so 并更新控制器加载，注意正逆解的单位一致性（um 与脉冲的转换）。

## 安全注意事项

1. **急停逻辑**: ESTOP 事件在所有状态下均可触发（通过 `SMF_DISPATCH` 路由到各状态处理器），触发后应立即停止所有任务并去使能轴。当前 `enter_estop()` 和 `reset_estop()` 调用已注释，需实现。
2. **心跳检测**: `safety_mgr.bas` 中的心跳超时检测尚为框架代码，通信丢失时的安全处理需完善。
3. **驱动器状态**: 应周期性读取 `DRIVE_STATUS` 和 `AXISSTATUS` 并在检测到异常时触发安全响应。
4. **回零要求**: 笛卡尔空间运动（CART_JOG、TRAJ）依赖正解获取绝对位姿，必须在回零完成后才能执行。FSM 在 `SYS_SERVO_READY` 状态下会拒绝这些指令。
5. **编码器单位**: `ENCODER_PER_ROE = 8388608 (2^23)`，丝杠导程 5mm，运动单位通过 `UNITS` 参数设置，注意驱动器电子齿轮比不能与 UNITS 重复缩放。
6. **限位**: 物理限位开关信号应接入控制器输入，代码中需添加限位检测逻辑。

## 测试策略

- `.bas` 文件中的桩任务函数（`HOME_TASK`、`JOINT_TASK`、`CART_JOG_TASK`、`TRAJ_TASK`）当前使用 `DELAY` 模拟执行，用于验证状态机逻辑
- FSM 测试：通过 MODBUS 写入不同事件，观察 `PRINT` 输出和 MODBUS 状态寄存器变化
- 回零测试：需连接实际驱动器，验证 DATUM 模式 21+29 的回零行为
- 运动学测试：在 ZDevelop 中通过 TABLE 写入测试坐标，调用 CFRAME 验证正逆解输出
- 调试输出：`frame1000.c` 中 `g_printflag` 控制逆解/正解的调试打印

## 当前开发状态

- [x] EtherCAT 总线初始化
- [x] FSM 状态机框架及全部状态处理器
- [x] 5 轴回零（home_robot）
- [x] 单轴手动控制（MANNUAL_JOINT）
- [x] 逆解（闭环矢量法，frame1000.c）
- [ ] 正解（当前为桩实现，返回固定坐标）
- [ ] 笛卡尔点动（CART_JOG_TASK 为桩函数）
- [x] 轨迹执行器（TRAJ_MOVE，支持 MOVE/MOVEABS/MOVE_PTABS/MOVE_DELAY，cmd_id=0 结束标记）
- [ ] 力控闭环微调（ctrL_mgr.bas：INT_CYCLE 周期任务，与轨迹共用数据区，每伺服周期消费一条 MOVE_PTABS 指令并下发（ticks=1）；FSM 已接入 EVENT_CTRL/EVENT_CTRL_DONE，读到 cmd_id=0 自行退出；传感器读取与微调算法待实现）
- [ ] 安全监控完整实现
- [ ] 限位检测逻辑

## 相关文档

- ZMC 编程手册：`../RTBasic编程手册V1.1.2.pdf`（项目父目录）
- 寄存器分配：`register_assignment.md`
- 历史版本：`../v2/` 的前身代码在父目录的 `motion1.bas`、`motion4.bas`、`home1.bas`、`trace1.bas` 等文件中
