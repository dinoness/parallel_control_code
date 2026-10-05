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
    GLOBAL CONST TABLE_CART_CMD_BEGIN = 350
    GLOBAL CONST TABLE_DRIVER_STATUE   = 999

    ' 状态监控环形区（status_mgr.bas 伺服周期采样，上位机 0.1s 批量读取）
    ' 每帧通道布局（STATUS_CHANNELS=24）：
    '   [0-4]  支链伸缩量 dL(um)，MPOS 相对 dl_base 基准
    '   [5-9]  电机编码器原始脉冲 ENCODER
    '   [10-14] 电机扭矩 DRIVE_TORQUE（0x6077，千分比 ‰）
    '   [15-19] 末端位姿 x,y,z,phi,theta（DPOS(6..10)，仅机器人模式有效）
    '   [20]   ee_valid 标志（1=末端位姿有效，0=无效）
    '   [21-23] 预留
    GLOBAL CONST TABLE_STATUS_BASE   = 21000  ' 状态监控环形区 TABLE 基址
    GLOBAL CONST STATUS_HEADER_FLOATS = 2     ' header：frame_counter + write_index
    GLOBAL CONST STATUS_RING_FRAMES  = 512    ' 环形帧数（200Hz 采样下覆盖 2.56s，为 0.1s 上传窗口留足链路争抢余量）
    GLOBAL CONST STATUS_CHANNELS     = 24     ' 每帧通道数
    GLOBAL CONST STATUS_SAMPLE_DIV   = 2      ' 采样分频：每 2 个伺服周期写一帧（1kHz 总线 → 200Hz），降低上传数据量


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
    GLOBAL CONST CMD_NONE        = 0  ' 无命令（轨迹中作为结束标记）
    GLOBAL CONST CMD_MOVE        = 1  '
    GLOBAL CONST CMD_MOVE_ABS    = 2
    GLOBAL CONST CMD_MOVE_PT     = 9
    GLOBAL CONST CMD_MOVE_PTABS  = 10  ' 单位时间绝对运动，第7字段为ticks
    GLOBAL CONST CMD_MOVE_DELAY  = 20  ' 缓冲延时，第7字段为延时毫秒数(ms)

    ' State flag of data
    GLOBAL CONST F_DataUpdate     = 1
    GLOBAL CONST F_DataUsed       = 2
    GLOBAL CONST F_DataBlank      = 3

    ' 速度等级代号
    GLOBAL CONST SPEED_L1 = 1
    GLOBAL CONST SPEED_L2 = 2
    GLOBAL CONST SPEED_L3 = 3

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
    GLOBAL CONST TASK_CTRL_MOVE     = 5
    GLOBAL CONST TASK_STATUS        = 6  ' 状态采样 INT_CYCLE 周期任务号


    ' error code
    GLOBAL CONST ERR_OK                = 0
    GLOBAL CONST ERR_UNKNOWN_CMD       = 1001
    GLOBAL CONST ERR_MODE_BUSY         = 1002
    GLOBAL CONST ERR_SYSTEM_NOT_READY  = 1003
    GLOBAL CONST ERR_NEED_HOME         = 1004
    GLOBAL CONST ERR_SAFETY_ACTIVE     = 1005
    GLOBAL CONST ERR_TASK_START_FAIL   = 1006
    GLOBAL CONST ERR_HOME_FAILED       = 1007

    

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
    GLOBAL CONST JOINT_L1_SPEED = 1 * LENGTH_UNIT
    GLOBAL CONST JOINT_L2_SPEED = 5 * LENGTH_UNIT
    GLOBAL CONST JOINT_L3_SPEED = 10 * LENGTH_UNIT
    GLOBAL CONST JOINT_L1_ACC   = 2 * LENGTH_UNIT
    GLOBAL CONST JOINT_L2_ACC   = 10 * LENGTH_UNIT
    GLOBAL CONST JOINT_L3_ACC   = 15 * LENGTH_UNIT
END SUB

