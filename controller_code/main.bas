' 状态监控环形区需 TABLE 至 21000+2+384*22=29450，默认 TSIZE 可能不足；
' TSIZE 重设会清空 TABLE，必须放在最前面（任何 TABLE 使用之前）
TSIZE = 34000

DELAY(3000)  '等待驱动器设备上电完成
PRINT "总线通讯周期：",SERVO_PERIOD,"us"

GLOBAL CONST BUS_NODE_NUM = 8  ' 6个电机驱动器 + EK1100（节点6）+ EL3702（节点7）
GLOBAL CONST BUS_SLOT = 0  ' 槽位号，默认0
GLOBAL CONST ROBO_PARA_START_ID = 0  ' 参数起始id



' 系统状态变量
GLOBAL system_state
GLOBAL motion_mode
GLOBAL motion_lock
GLOBAL safety_state
GLOBAL active_task
GLOBAL last_cmd_seq
' GLOBAL cmd_id  ' 后续的子模块中有调用，这里就不用了
GLOBAL bus_initstate  ' 总线初始化状态
GLOBAL bus_total_axis_num

' 末端工具命令状态；工具控制不改变机器人FSM状态
GLOBAL revolution_running
GLOBAL rotation_running
GLOBAL revolution_speed_rpm
GLOBAL rotation_speed_rpm

' EL3702 硬件采集状态；仅在周期中断中更新，不新增任务
' 采样数据直写全局变量，TABLE(401/402/405) 仅作 10ms 低频调试镜像
GLOBAL force_hw_sample_count    ' 硬件采样序号（每伺服周期+1，供断流检测）
GLOBAL force_hw_valid           ' 硬件数据有效标志（节点在线即有效，10ms 刷新）
GLOBAL force_enabled            ' 适配启停缓存（跟随 TABLE(400)，10ms 刷新）

GLOBAL_DEF()
REG_CLEAR()
'========================================================
' 系统状态初始化
'========================================================
system_state = SYS_BOOT
motion_mode = MODE_IDLE
motion_lock = MODE_IDLE
active_task = -1
safety_state = 0
revolution_running = 0
rotation_running = 0
revolution_speed_rpm = 0
rotation_speed_rpm = 0
force_hw_sample_count = 0
force_hw_valid = 0
force_enabled = 0

MODBUS_REG(REG_SYSTEM_STATE) = system_state
MODBUS_REG(REG_MOTION_MODE) = motion_mode
' MODBUS_REG(REG_CMD_STATE) = CMD_STATE_IDLE
MODBUS_REG(REG_CMD_ERROR) = ERR_OK
last_cmd_seq = MODBUS_REG(REG_CMD_SEQ)

' 自转输出上电先关闭，避免总线初始化期间残留PWM或使能信号
TOOL_ROTATION_INIT()

'========================================================
' EtherCAT通讯
'========================================================
system_state = SYS_BUS_INIT
MODBUS_REG(REG_SYSTEM_STATE) = system_state


bus_initstate = -1

Ecat_Init()
WHILE (bus_initstate = 0)
    Ecat_Init()
WEND


system_state = SYS_SERVO_READY
MODBUS_REG(REG_SYSTEM_STATE) = SYS_SERVO_READY


'========================================================
' 运动参数配置
'========================================================
'' ==========  设置运动相关参数  ==========
AXIS_CONFIG()

'' ==========  定义几何尺寸  ==========
' 结构参数表按5个机器人轴布局，不能包含公转轴5
ROBO_PARA_CONFIG1(5)

'' ==========  定义机械手  ==========
' 机器人运动学始终只包含0-4五个实轴，公转轴5不参与CFRAME
DEFINE_CFRAME  1000,5,0,0,0    'framenum, totalaxises, axises_aux,  max_attitudes,  rotatetype

'' ==========  CTRL 混合轨迹与力控变量（必须在周期中断注册前声明）  ==========
GLOBAL nominal_pos(5)       ' 不含力控修正的名义位姿，0-4: x,y,z,theta,phi
GLOBAL adj_pos(5)           ' 力控修正量，当前仅使用 Z 分量 adj_pos(2)
GLOBAL has_target           ' 力控段已读取到有效 PT 目标
GLOBAL ctrl_index           ' 下一条待读指令的 TABLE 索引
GLOBAL traj_over            ' 已读到 CMD_NONE，等待全部运动结束
GLOBAL ctrl_running         ' 整条 CTRL 混合轨迹运行标志
GLOBAL ctrl_paused          ' 混合轨迹暂停标志，周期采样仍继续
GLOBAL exec_state           ' EXEC_NORMAL / EXEC_FORCE / 各等待状态

GLOBAL force_active         ' 当前是否处于显式力控段
GLOBAL force_state          ' IDLE / SEARCH / HOLD / FAULT
GLOBAL force_cfg_mode       ' 下一段或当前段的力控模式
GLOBAL force_target_cmd     ' 指令给定的目标力(N)
GLOBAL force_target_effective ' 经斜坡限制后的实际目标力(N)
GLOBAL force_contact_threshold ' 接触判定阈值(N)
GLOBAL force_ramp_rate      ' 目标力变化斜率(N/s)
GLOBAL force_adjust_limit   ' 当前段修正量限幅(um)
GLOBAL force_contact_count  ' 接触阈值连续成立周期数
GLOBAL force_prev_error     ' 上一周期力误差
GLOBAL buffer_wait_ticks    ' 力控段等待新数据组或运动缓冲的周期数

GLOBAL force_raw(6)         ' Fx,Fy,Fz,Mx,My,Mz 原始值
GLOBAL force_normal         ' 标度、去皮后的瞬时法向力(N)
GLOBAL force_filtered       ' 一阶低通后的法向力(N)
GLOBAL force_sensor_status  ' 传感器有效、无效、断流、过力状态
GLOBAL force_last_sample_count ' 上一次硬件采样序号
GLOBAL force_stale_ticks    ' 硬件采样序号未更新的连续周期数

ctrl_running = 0

'' ==========  开启中断功能  ==========
INT_ENABLE = 1

' 启动统一伺服周期任务（状态采样常驻，CTRL 混合轨迹由 ctrl_running 控制）
' 必须在 AXIS_CONFIG() 之后调用：dl_base 基准需在 UNITS 设置后采集，保证 dL 单位为 um
STATUS_INIT()


'========================================================
' 主循环
'========================================================
DIM cur_event
WHILE 1

    ' 2. 获取新事件
    cur_event = CHECK_EVENT(REG_EVENT_BEGIN, MAX_EVENT_LEVEL)

    ' 3. 监控当前任务
    SMF_DISPATCH(system_state, cur_event)

    ' 4. 写状态反馈
    MODBUS_REG(REG_SYSTEM_STATE) = system_state' 系统状态
    MODBUS_REG(REG_MOTION_MODE) = motion_mode  ' 运动模式
    MODBUS_REG(REG_ACTIVE_TASK) = active_task  ' 当前正在执行的任务

    DELAY(100)

WEND