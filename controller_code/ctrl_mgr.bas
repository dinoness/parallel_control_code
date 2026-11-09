' ================================================================
' ctrl_mgr.bas - 伺服周期调度、状态采样与力传感器闭环微调运动
' 周期调度：
'   全工程仅使用 TASK_CTRL 注册一个 INT_CYCLE，每个 SERVO_PERIOD 调用 SERVO_CYCLE。
'   SERVO_CYCLE 始终执行状态采样，仅在 ctrl_running=1 时执行闭环控制。
' 状态采样：
'   每 STATUS_SAMPLE_DIV 个伺服周期写入 TABLE 状态环形区，供上位机批量读取。
'   数据区：TABLE_STATUS_BASE(21000) 起，header 2 个 float + 512 帧 × 24 通道。
'   同步约定：先写完一帧数据，最后更新 header，保证上位机不会读到半成品帧。
' 闭环数据协议：与轨迹执行（traj_move_mgr.bas）共用数据区和组状态寄存器
'   指令区: TABLE_TRAJ_BEGIN(1000) 起，10 个指令组，每组 100 条，每条 7 个数：
'     cmd_id, x, y, z, theta, phi, ticks(闭环模式下不用)
'     闭环仅支持 cmd_id = CMD_MOVE_PTABS(10)，CMD_NONE(0) 为结束标记
'   组状态: MODBUS_REG(REG_TRACE_CMD_STATE_ST + 组号)
'     F_DataUpdate=1 上位机已写入, F_DataUsed=2 控制器已消费
' 闭环运行方式：
'   CTRL_MOVE() 打开闭环分支，CTRL_MOVE_END() 关闭闭环分支。
'   每个伺服周期最多读取一条指令并下发一次 MOVE_PTABS（ticks 固定为 1），
'   组数据未就绪时保持上一目标位姿，不做阻塞等待和复杂运算。
'   读到 CMD_NONE 或非法指令时关闭闭环分支，并通知 FSM 退出闭环模式。
'   运动对象为并联机构虚拟轴（6-10），CONNFRAME 由 FSM 进入机器人模式时建立。
' 全局变量声明位置：
'   状态采样变量在 global_config.bas；闭环变量在 main.bas，均须在中断注册前声明。
' ================================================================


' 初始化状态采样并注册全工程唯一的伺服周期中断
GLOBAL SUB STATUS_INIT()
    PRINT "状态采样任务启动"
    DL_BASE_CAPTURE()
    ee_valid = 0
    status_frame_counter = 0
    status_write_index = 0
    status_tick = 0
    ctrl_running = 0
    ' header 清零
    TABLE(TABLE_STATUS_BASE) = 0
    TABLE(TABLE_STATUS_BASE + 1) = 0
    ' 注册统一周期入口（INT_ENABLE=1 已在 main.bas 全局打开）
    INT_CYCLE(1, TASK_CTRL, SERVO_CYCLE)
END SUB

' 统一伺服周期入口：闭环控制优先，状态采样常驻
GLOBAL SUB SERVO_CYCLE()
    IF ctrl_running = 1 THEN
        PID_MOVE()
    ENDIF
    STATUS_SAMPLE()
END SUB

' 标定 dL 零位基准：回零完成后调用，使 dL 以回零点为零位
GLOBAL SUB DL_BASE_CAPTURE()
    LOCAL i
    FOR i = 0 TO 4
        dl_base(i) = MPOS(i)
    NEXT
END SUB

' 状态采样：每 STATUS_SAMPLE_DIV 个伺服周期写一帧
GLOBAL SUB STATUS_SAMPLE()
    LOCAL i, base_addr

    status_tick = status_tick + 1
    IF status_tick >= STATUS_SAMPLE_DIV THEN
        status_tick = 0
        base_addr = TABLE_STATUS_BASE + STATUS_HEADER_FLOATS + status_write_index * STATUS_CHANNELS

        FOR i = 0 TO 4
            TABLE(base_addr + i) = MPOS(i) - dl_base(i)          ' [0-4] 支链伸缩量 dL(um)
            ' 若实测 ATYPE=65 不支持 ENCODER，改用 MPOS(i)*UNITS(i)
            TABLE(base_addr + 5 + i) = ENCODER(i)                ' [5-9] 电机编码器原始脉冲
            TABLE(base_addr + 10 + i) = DRIVE_TORQUE(i)          ' [10-14] 电机扭矩（‰）
        NEXT

        ' [15-20] 末端位姿仅在 CONNFRAME 已建立时读取
        IF ee_valid = 1 THEN
            FOR i = 0 TO 4
                TABLE(base_addr + 15 + i) = DPOS(6 + i)
            NEXT
            TABLE(base_addr + 20) = 1
        ELSE
            TABLE(base_addr + 20) = 0
        ENDIF
        ' [21-23] 预留，不写

        ' 环形推进写指针，帧计数递增
        status_write_index = (status_write_index + 1) MOD STATUS_RING_FRAMES
        status_frame_counter = status_frame_counter + 1
        ' 最后写 header，保证上位机读到的帧数据完整
        TABLE(TABLE_STATUS_BASE, status_frame_counter, status_write_index)
    ENDIF
END SUB


' 启动闭环分支：初始化完成后才允许周期入口执行 PID_MOVE
GLOBAL SUB CTRL_MOVE()
    PRINT "闭环控制运动启动"
    has_target = 0
    traj_over = 0
    ctrl_index = TABLE_TRAJ_BEGIN
    ctrl_running = 1
END SUB

' 停止闭环分支：保留周期中断和状态采样，只禁止后续闭环处理
GLOBAL SUB CTRL_MOVE_END()
    ctrl_running = 0
    has_target = 0
    PRINT "闭环控制运动停止"
END SUB

' 暂停闭环分支：保留目标位姿和轨迹索引，供恢复后继续
GLOBAL SUB CTRL_MOVE_PAUSE()
    ctrl_running = 0
END SUB

' 恢复闭环分支：已结束的轨迹不恢复
GLOBAL SUB CTRL_MOVE_RESUME()
    IF traj_over = 0 THEN
        ctrl_running = 1
    ENDIF
END SUB

' 周期闭环入口：读取指令、传感器采集、控制微调、轨迹下发
GLOBAL SUB PID_MOVE()
    LOCAL i, cur_group, group_offset, cmd_id, can_read

    ' 1. 读取上位机指令：每周期最多从轨迹缓冲区消费一条指令
    can_read = 0
    IF traj_over = 0 THEN
        group_offset = (ctrl_index - TABLE_TRAJ_BEGIN) MOD SIZE_TRAJ_BLOCK
        cur_group = (ctrl_index - TABLE_TRAJ_BEGIN) \ SIZE_TRAJ_BLOCK
        ' 新组起始需上位机已写入；未就绪则本周期不消费，保持上一目标
        IF group_offset = 0 THEN
            IF MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUpdate THEN
                can_read = 1
            ENDIF
        ELSE
            can_read = 1
        ENDIF
        IF can_read = 1 THEN
            cmd_id = TABLE(ctrl_index)
            IF cmd_id = CMD_MOVE_PTABS THEN
                FOR i = 0 TO 4
                    cmd_pos(i) = TABLE(ctrl_index + 1 + i)
                NEXT
                has_target = 1
            ELSEIF cmd_id = CMD_MOVE_PT THEN
                 
            ELSEIF cmd_id = CMD_NONE THEN
                ' 轨迹结束标记：释放当前组，关闭闭环分支并通知 FSM
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                traj_over = 1
                MODBUS_REG(REG_EVENT_L1) = EVENT_CTRL_DONE
                CTRL_MOVE_END()
            ELSE
                PRINT "[CTRL] 非法指令代号: ", cmd_id, " TABLE索引: ", ctrl_index
                MODBUS_REG(REG_CMD_ERROR) = ERR_UNKNOWN_CMD
                traj_over = 1
                MODBUS_REG(REG_EVENT_L1) = EVENT_CTRL_DONE
                CTRL_MOVE_END()
            ENDIF
            ' 推进索引，跨组时释放当前组并回绕到下一组
            ctrl_index = ctrl_index + SIZE_TRAJ_CMD
            IF group_offset + SIZE_TRAJ_CMD >= SIZE_TRAJ_BLOCK THEN
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                ctrl_index = TABLE_TRAJ_BEGIN + ((cur_group + 1) MOD NUM_TRACE_DATA_GROUP) * SIZE_TRAJ_BLOCK
            ENDIF
        ENDIF
    ENDIF

    ' 2. 读取力传感器信号（待实现：传感器通道与数据区确定后补充）

    ' 3. 力控微调：根据力误差计算位姿偏移 adj_pos（待实现，当前恒为 0）

    ' 4. 轨迹下发：目标位姿 + 微调偏移，每周期最多下发一次
    IF has_target = 1 THEN
        BASE(6,7,8,9,10)
        MOVE_PTABS(1, cmd_pos(0)+adj_pos(0), cmd_pos(1)+adj_pos(1), cmd_pos(2)+adj_pos(2), cmd_pos(3)+adj_pos(3), cmd_pos(4)+adj_pos(4))
    ENDIF
END SUB
