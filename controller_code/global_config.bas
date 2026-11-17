GLOBAL SUB GLOBAL_DEF()
    ' ======================================================
    ' Register Assignment
    ' ======================================================
    ' modbus reg config
    GLOBAL CONST REG_PC_HEARTBEAT            = 0  ' Qt心跳计数？
    GLOBAL CONST REG_CMD_SEQ                 = 1  ' 命令序号计数，每发一套命令+1
    GLOBAL CONST REG_CMD_ID                  = 2
    GLOBAL CONST REG_CMD_STATE               = 3
    GLOBAL CONST REG_CMD_ERROR               = 4
    GLOBAL CONST REG_SYSTEM_STATE            = 5
    GLOBAL CONST REG_MOTION_MODE             = 6
    GLOBAL CONST REG_SAFETY_STATE            = 7
    GLOBAL CONST REG_ACTIVE_TASK             = 8
	GLOBAL CONST REG_TRACE_CMD_STATE_ST      = 50
	GLOBAL CONST REG_CART_CMD_STATE_ST       = 70
    GLOBAL CONST REG_JOINT_CMD_STATE_ST      = 80
    GLOBAL CONST REG_EVENT_BEGIN             = 90
	GLOBAL CONST REG_EVENT_L0                = 90
	GLOBAL CONST REG_EVENT_L1                = 91
	GLOBAL CONST REG_EVENT_L2                = 92

    ' table id config
    GLOBAL CONST TABLE_ROBO_PARA_BEGIN = 0
    GLOBAL CONST TABLE_JOINT_CMD_BEGIN = 300
    GLOBAL CONST TABLE_CART_CMD_BEGIN  = 350
    GLOBAL CONST TABLE_DRIVER_STATUE   = 999

    ' 末端工具指令区与公转轴参数。
    ' ATYPE=66 时通过 DAC 调速，EtherCAT 速度模式的 DAC 单位为脉冲数/秒。
    ' 公转电机为23位编码器，直驱且驱动器无额外电子齿轮缩放时，每转为2^23脉冲。
    GLOBAL CONST TABLE_REVOLUTION_SPEED = 450
    GLOBAL CONST TABLE_ROTATION_SPEED   = 451
    GLOBAL CONST TOOL_REVOLUTION_AXIS   = 5
    GLOBAL CONST MAX_TOOL_SPEED_RPM     = 2000
    GLOBAL CONST TOOL_REV_PULSES_PER_REV = 8388608.0
    GLOBAL CONST TOOL_DAC_PER_RPM       = TOOL_REV_PULSES_PER_REV / 60.0

    ' 自转步进电机：ZMC406-V2高速OUT0输出硬件PWM，OUT4方向，OUT5使能。
    ' 每转3200脉冲，2000rpm对应106666.67Hz，低于OUT0的400kHz硬件上限。
    GLOBAL CONST TOOL_ROT_PULSES_PER_REV = 3200.0
    GLOBAL CONST TOOL_ROT_PULSE_OUT      = 0
    GLOBAL CONST TOOL_ROT_DIR_OUT        = 4
    GLOBAL CONST TOOL_ROT_ENABLE_OUT     = 5
    GLOBAL CONST TOOL_ROT_AUX_AXIS       = 6  ' OUT0复用AXIS6，须设为虚拟轴释放普通输出
    GLOBAL CONST TOOL_ROT_PWM_DUTY       = 0.5
    GLOBAL CONST TOOL_ROT_ENABLE_MS      = 10
    GLOBAL CONST TOOL_ROT_DIR_MS         = 1

    ' 力传感器适配与控制参数，TABLE 400-419。
    ' 控制路径只读参数（enable/标定/滤波/PI/限幅/阈值）；valid/sample_count/原始值
    ' 仅为 10ms 低频调试镜像（供 ZDevelop 观察），控制与断流检测走全局变量。
    ' 第一阶段 enable=1 用于采集显示，PI 增益保持0，不进行闭环力控。
    GLOBAL CONST TABLE_FORCE_CFG_BEGIN   = 400
    GLOBAL CONST FORCE_CFG_ENABLE        = 0   ' 1=启用传感器适配层（10ms 刷新进 force_enabled）
    GLOBAL CONST FORCE_CFG_VALID         = 1   ' 调试镜像：1=节点在线（10ms 刷新）
    GLOBAL CONST FORCE_CFG_SAMPLE_COUNT  = 2   ' 调试镜像：采样序号（10ms 刷新）
    GLOBAL CONST FORCE_CFG_RAW_BEGIN     = 3   ' 3-8: Fx,Fy,Fz,Mx,My,Mz 原始值（仅 405=Fz 作调试镜像，10ms 刷新）
    GLOBAL CONST FORCE_CFG_SCALE         = 9   ' Fz 原始值到 N 的比例，可带符号
    GLOBAL CONST FORCE_CFG_ZERO          = 10  ' Fz 空载原始零位
    GLOBAL CONST FORCE_CFG_FILTER_ALPHA  = 11  ' 一阶低通系数，合法范围 (0,1] 由参数配置侧保证
    GLOBAL CONST FORCE_CFG_KP            = 12  ' 增量式 PI 比例系数
    GLOBAL CONST FORCE_CFG_KI            = 13  ' 增量式 PI 积分系数
    GLOBAL CONST FORCE_CFG_MAX_FORCE     = 14  ' 过力保护阈值(N)
    GLOBAL CONST FORCE_CFG_MAX_ADJUST    = 15  ' 修正量硬限幅(um)
    GLOBAL CONST FORCE_CFG_MAX_ADJ_RATE  = 16  ' 修正速度限幅(um/s)
    GLOBAL CONST FORCE_CFG_CONTACT_TICKS = 17  ' 接触阈值连续成立周期数
    GLOBAL CONST FORCE_CFG_DIRECTION     = 18  ' Z 修正方向，只能为 +1 或 -1
    GLOBAL CONST FORCE_CFG_STALE_TICKS   = 19  ' 断流阈值，单位 10ms（断流检测在 10ms 低频段执行）

    ' 总线节点0-5为驱动器，节点6为EK1100，节点7为EL3702；Fz 接 EL3702 CH1。
    ' 仅使用标准 AIN。总线周期 1ms 与伺服周期 1:1，每周期必有新记录，不读 CycleCounter；
    ' EL3702 原生输出有符号 INT16（±10V 对应 ±32767），ZMC 侧 AIN 若按无符号返回，
    ' 由 FORCE_SAMPLE 用算术回绕恢复，无需端子侧配置。
    ' 注意：EL3702 为超采样端子，超采样系数须为 1，否则 AIN(33) 只是块内第一个（最旧）采样。
    GLOBAL CONST FORCE_NODE              = 7
    GLOBAL CONST FORCE_AIN_BASE          = 32  ' AIN(33)=CH1模拟量；AIN(32)=CycleCounter 不再读取；起始编号须为8的倍数
    GLOBAL CONST FORCE_FZ_FULL_SCALE_N   = 49.03325 ' 5kgf，仅作理论标定初值
    GLOBAL CONST FORCE_ADC_FULL_SCALE    = 32768    ' EL3702 ±10V 对应有符号16位满量程
    GLOBAL CONST FORCE_FZ_DEFAULT_SCALE  = FORCE_FZ_FULL_SCALE_N / FORCE_ADC_FULL_SCALE

    ' 状态监控环形区：384 帧 × 22 通道
    GLOBAL CONST TABLE_STATUS_BASE    = 21000
    GLOBAL CONST STATUS_HEADER_FLOATS = 2     ' frame_counter + write_index
    GLOBAL CONST STATUS_RING_FRAMES   = 384   ' 100Hz 下覆盖 3.84s
    GLOBAL CONST STATUS_CHANNELS      = 22
    GLOBAL CONST STATUS_SAMPLE_DIV    = 10    ' 每 10ms 在一个伺服周期内批量采集并提交完整帧(100Hz)


    ' ======================================================
    ' Protocol Para
    ' ======================================================
    ' system_state
    GLOBAL CONST SYS_BOOT          = 0
    GLOBAL CONST SYS_BUS_INIT      = 1
    GLOBAL CONST SYS_SERVO_READY   = 2
    GLOBAL CONST SYS_HOMING        = 3
    GLOBAL CONST SYS_READY         = 4  ' 已回零，可运动
    GLOBAL CONST SYS_ROBOT_MODE    = 5
    GLOBAL CONST SYS_RUNNING       = 6
    GLOBAL CONST SYS_PAUSED        = 17
    GLOBAL CONST SYS_ERROR         = 18
    GLOBAL CONST SYS_ESTOP         = 19  ' 急停

    ' event id(事件)
    GLOBAL CONST EVENT_IDLE          = 0
    GLOBAL CONST EVENT_HOME          = 1
    GLOBAL CONST EVENT_HOME_DONE     = 2
    GLOBAL CONST EVENT_JOINT         = 3
    GLOBAL CONST EVENT_JOINT_DONE    = 4
    GLOBAL CONST EVENT_CART_JOG      = 5
    GLOBAL CONST EVENT_CART_JOG_DONE = 6
    GLOBAL CONST EVENT_TRAJ          = 7
    GLOBAL CONST EVENT_TRAJ_DONE     = 8
    GLOBAL CONST EVENT_CTRL          = 9
    GLOBAL CONST EVENT_CTRL_DONE     = 10
    GLOBAL CONST EVENT_REVOLUTION_START = 11
    GLOBAL CONST EVENT_REVOLUTION_STOP  = 12
    GLOBAL CONST EVENT_ROTATION_START   = 13
    GLOBAL CONST EVENT_ROTATION_STOP    = 14
    GLOBAL CONST EVENT_ROBOT_IN      = 21
    GLOBAL CONST EVENT_ROBOT_OUT     = 22
    GLOBAL CONST EVENT_STOP          = 81
    GLOBAL CONST EVENT_PAUSE         = 82
    GLOBAL CONST EVENT_RESUME        = 83
    GLOBAL CONST EVENT_ERROR_RESET   = 90
    GLOBAL CONST EVENT_HOME_FAILED   = 91
    GLOBAL CONST EVENT_ESTOP         = 99

    ' motion_mode
    GLOBAL CONST MODE_IDLE             = 0  ' 空闲
    GLOBAL CONST MODE_JOINT_MANUAL     = 1  ' 手动控制
    GLOBAL CONST MODE_HOME             = 2
    GLOBAL CONST MODE_CART_JOG         = 3  ' 笛卡尔方向点动
    GLOBAL CONST MODE_TRAJECTORY       = 4
    GLOBAL CONST MODE_SENSOR_CLOSED    = 5

    ' motion_cmd_id
    GLOBAL CONST CMD_NONE        = 0
    GLOBAL CONST CMD_MOVE        = 1
    GLOBAL CONST CMD_MOVE_ABS    = 2
    GLOBAL CONST CMD_MOVE_PT     = 9
    GLOBAL CONST CMD_MOVE_PTABS  = 10
    GLOBAL CONST CMD_MOVE_DELAY  = 20
    GLOBAL CONST CMD_FORCE_CFG   = 30  ' mode,target_N,contact_N,ramp_N_s,max_adjust_um,reserved
    GLOBAL CONST CMD_FORCE_OFF   = 31

    ' CTRL 混合轨迹执行状态
    GLOBAL CONST EXEC_NORMAL           = 0
    GLOBAL CONST EXEC_WAIT_NORMAL_IDLE = 1
    GLOBAL CONST EXEC_FORCE            = 2
    GLOBAL CONST EXEC_WAIT_FORCE_IDLE  = 3
    GLOBAL CONST EXEC_WAIT_ALL_IDLE    = 4

    GLOBAL CONST FORCE_MODE_IDLE   = 0
    GLOBAL CONST FORCE_MODE_SEARCH = 1
    GLOBAL CONST FORCE_MODE_HOLD   = 2
    GLOBAL CONST FORCE_MODE_FAULT  = 3

    GLOBAL CONST FORCE_SENSOR_DISABLED  = 0
    GLOBAL CONST FORCE_SENSOR_VALID     = 1
    GLOBAL CONST FORCE_SENSOR_INVALID   = 2
    GLOBAL CONST FORCE_SENSOR_STALE     = 3
    GLOBAL CONST FORCE_SENSOR_OVERLIMIT = 4

    ' State flag of data
    GLOBAL CONST F_DataUpdate     = 1
    GLOBAL CONST F_DataUsed       = 2
    GLOBAL CONST F_DataBlank      = 3

    ' Mannual Joint
    GLOBAL CONST SIZE_JOINT_CMD = 7
    GLOBAL CONST SIZE_CART_CMD  = 7

    ' Trajectory
    GLOBAL CONST NUM_TRACE_DATA_GROUP    = 10
    GLOBAL CONST TABLE_TRAJ_BEGIN        = 1000  ' 轨迹数据 TABLE 起始地址
    GLOBAL CONST SIZE_TRAJ_CMD           = 7     ' 一条指令的数据个数
    GLOBAL CONST NUM_TRAJ_CMD_PER_GROUP  = 100   ' 每组指令数
    GLOBAL CONST SIZE_TRAJ_BLOCK         = SIZE_TRAJ_CMD * NUM_TRAJ_CMD_PER_GROUP  ' 每组占700个TABLE位置


    ' ======================================================
    ' Para in Controller
    ' ======================================================
    ' task id(额外开启的任务号)
    GLOBAL CONST TASK_HOEM          = 1
    GLOBAL CONST TASK_JOINT         = 2
    GLOBAL CONST TASK_CATR_JOG      = 3
    GLOBAL CONST TASK_TRAJ          = 4
    GLOBAL CONST TASK_CTRL          = 5  ' 统一伺服周期任务：状态采样与闭环控制共用


    ' error code
    GLOBAL CONST ERR_OK                   = 0
    GLOBAL CONST ERR_UNKNOWN_CMD          = 1001
    GLOBAL CONST ERR_MODE_BUSY            = 1002
    GLOBAL CONST ERR_SYSTEM_NOT_READY     = 1003
    GLOBAL CONST ERR_NEED_HOME            = 1004
    GLOBAL CONST ERR_SAFETY_ACTIVE        = 1005
    GLOBAL CONST ERR_TASK_START_FAIL      = 1006
    GLOBAL CONST ERR_HOME_FAILED          = 1007
    GLOBAL CONST ERR_FORCE_SENSOR_INVALID = 1008
    GLOBAL CONST ERR_FORCE_OVERLIMIT      = 1009
    GLOBAL CONST ERR_FORCE_CMD_INVALID    = 1010
    GLOBAL CONST ERR_FORCE_BUFFER_TIMEOUT = 1011
    GLOBAL CONST ERR_FORCE_PARAM_INVALID  = 1012
    GLOBAL CONST ERR_TOOL_RUNNING         = 1013
    GLOBAL CONST ERR_TOOL_SPEED_INVALID   = 1014

    

    ' other config para
    GLOBAL CONST MAX_EVENT_LEVEL = 3
	
	' 采集相关数据
	GLOBAL dl_base(5)            ' 各实轴 dL 零位基准（回零后重新标定）
	GLOBAL ee_valid              ' 末端位姿有效标志（1=机器人模式已建立，DPOS(6..10)可读）
	GLOBAL status_frame_counter  ' 已写入的总帧数（单调递增）
	GLOBAL status_write_index    ' 下一帧写入位置（0..STATUS_RING_FRAMES-1，环形）
	GLOBAL status_tick           ' 采样分频计数器（每 STATUS_SAMPLE_DIV 个伺服周期写一帧）

END SUB

GLOBAL SUB REG_CLEAR()
	LOCAL i
	' FOR i = 0 TO (NUM_JOINT_DATA_GROUP - 1) STEP 1
	' 	MODBUS_REG(REG_JOINT_CMD_STATE_ST + i) = 0
	' NEXT
	
	' FOR i = 0 TO (NUM_JOG_DATA_GROUP - 1) STEP 1
	' 	MODBUS_REG(REG_JOG_CMD_STATE_BEGIN + i) = 0
	' NEXT


	MODBUS_REG(REG_JOINT_CMD_STATE_ST) = 0
	MODBUS_REG(REG_CART_CMD_STATE_ST) = 0
	
	FOR i = 0 TO (NUM_TRACE_DATA_GROUP - 1) STEP 1
		MODBUS_REG(REG_TRACE_CMD_STATE_ST + i) = 0
	NEXT
	
	
END SUB


' 配置运动相关的参数
GLOBAL SUB AXIS_CONFIG()
    GLOBAL CONST LENGTH_UNIT = 1000  ' 长度单位转化，1代表mm，1000代表um
    GLOBAL CONST PB = 5 * LENGTH_UNIT  ' 丝杠导程
    GLOBAL CONST ENCODER_PER_ROE = 8388608  ' 2^23

    GLOBAL CONST u_j1 =  ENCODER_PER_ROE / PB  ' 关节1实际1mm or um脉冲数
    GLOBAL CONST u_j2 =  ENCODER_PER_ROE / PB  ' 关节2实际1mm or um脉冲数
    GLOBAL CONST u_j3 =  ENCODER_PER_ROE / PB  ' 关节3实际1mm or um脉冲数
    GLOBAL CONST u_j4 =  ENCODER_PER_ROE / PB  ' 关节4实际1mm or um脉冲数
    GLOBAL CONST u_j5 =  ENCODER_PER_ROE / PB  ' 关节5实际1mm or um脉冲数
    ' UNITS为指定运行一个单位需要的脉冲数，之后所有的运动指令都以此为单位
    ' 经过实测，电机运行一圈的脉冲数就是编码器一圈的数值，前提是驱动器中没有设置电子齿轮
    ' 不仅是脉冲轴，总线轴也要设置UNITS

    BASE(0,1,2,3,4)
    UNITS = u_j1, u_j2, u_j3, u_j4, u_j5

    

    ' 速度依然以mm作为定义
    ' 手动运动（单轴/点动）速度由上位机以 mm/s 直接下发，此处限幅最大值
    GLOBAL CONST MAX_MANUAL_SPEED = 20  ' 手动运动最大速度(mm/s)
    GLOBAL CONST JOINT_L1_SPEED = 1 * LENGTH_UNIT
    GLOBAL CONST JOINT_L2_SPEED = 5 * LENGTH_UNIT
    GLOBAL CONST JOINT_L3_SPEED = 10 * LENGTH_UNIT
    GLOBAL CONST JOINT_L1_ACC   = 2 * LENGTH_UNIT
    GLOBAL CONST JOINT_L2_ACC   = 10 * LENGTH_UNIT
    GLOBAL CONST JOINT_L3_ACC   = 15 * LENGTH_UNIT
END SUB

