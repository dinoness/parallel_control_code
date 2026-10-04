' ================================================================
' ctrL_mgr.bas - 力传感器闭环微调运动（周期任务）
' 数据协议：与轨迹执行（traj_move_mgr.bas）共用数据区和组状态寄存器
'   指令区: TABLE_TRAJ_BEGIN(1000) 起，10 个指令组，每组 100 条，每条 7 个数：
'     cmd_id, x, y, z, theta, phi, ticks(闭环模式下不用)
'     闭环仅支持 cmd_id = CMD_MOVE_PTABS(10)，CMD_NONE(0) 为结束标记
'   组状态: MODBUS_REG(REG_TRACE_CMD_STATE_ST + 组号)
'     F_DataUpdate=1 上位机已写入, F_DataUsed=2 控制器已消费
' 运行方式：
'   CTRL_MOVE() 启动周期闭环，CTRL_MOVE_END() 停止。
'   底层使用 INT_CYCLE 中断周期执行，每个 SERVO_PERIOD 调用一次 PID_MOVE。
'   手册要求周期执行的 SUB 必须足够精简——指单次执行的实际计算量要小：
'   本实现每周期最多读取一条指令、下发一次 MOVE_PTABS，不做阻塞等待和复杂运算。
'   （拆分为子函数只为结构清晰，并不减少计算量。）
'   每个伺服周期消费一条指令并下发一次 MOVE_PTABS（ticks 固定为 1），
'   组数据未就绪时保持上一目标位姿。
'   退出条件：读到结束标记 cmd_id=0（CMD_NONE）时自行调用 CTRL_MOVE_END()，
'   并向 REG_EVENT_L1 写入 EVENT_CTRL_DONE 通知 FSM 退出闭环模式。
'   运动对象为并联机构虚拟轴（6-10），CONNFRAME 由 FSM 进入机器人模式时建立。
' ================================================================



' 启动闭环周期任务
GLOBAL SUB CTRL_MOVE()
    PRINT "闭环控制运动启动"
    has_target = 0
    traj_over = 0
    ctrl_index = TABLE_TRAJ_BEGIN
    INT_ENABLE = 1
    INT_CYCLE(1, TASK_CTRL_MOVE, PID_MOVE)
END SUB

' 停止闭环周期任务
GLOBAL SUB CTRL_MOVE_END()
    PRINT "闭环控制运动停止"
    INT_CYCLE(2, TASK_CTRL_MOVE, PID_MOVE)
END SUB

' 周期闭环入口：每个 SERVO_PERIOD 执行一次，必须精简
GLOBAL SUB PID_MOVE()
    Read_Ctrl_Cmd()     ' 读取指令
    Read_Force()        ' 读取传感器信号
    Force_Tune()        ' 控制微调
    Send_Target()       ' 轨迹下发
END SUB

' 读取上位机指令：每周期从轨迹缓冲区消费一条指令 -> cmd_pos
SUB Read_Ctrl_Cmd()
    LOCAL i, cur_group, group_offset, cmd_id, can_read
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
            ELSEIF cmd_id = CMD_NONE THEN
                ' 轨迹结束标记：释放当前组，停止周期任务并通知 FSM 退出闭环模式
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
END SUB

' 读取力传感器信号（待实现：传感器通道与数据区确定后补充）
SUB Read_Force()
END SUB

' 力控微调：根据力误差计算位姿偏移 adj_pos（待实现，当前恒为0）
SUB Force_Tune()
END SUB

' 轨迹下发：目标位姿 + 微调偏移，每周期下发一次 MOVE_PTABS（ticks=1）
SUB Send_Target()
    IF has_target = 1 THEN
        BASE(6,7,8,9,10)
        MOVE_PTABS(1, cmd_pos(0)+adj_pos(0), cmd_pos(1)+adj_pos(1), cmd_pos(2)+adj_pos(2), cmd_pos(3)+adj_pos(3), cmd_pos(4)+adj_pos(4))
    ENDIF
END SUB
