' ================================================================
' traj_move_mgr.bas - 轨迹运动执行
' 数据协议（见 command_map.md / register_assignment.md）：
'   TABLE 1000 起，10 个指令组，每组 100 条指令，每条 7 个数：
'     cmd_id, x, y, z, theta, phi, ticks/blank
'   cmd_id: 1=MOVE 相对直线, 2=MOVEABS 绝对直线,
'           10=MOVE_PTABS 单位时间绝对(ticks为第7字段),
'           20=MOVE_DELAY 缓冲延时(第7字段为延时毫秒数ms),
'           0=CMD_NONE 轨迹结束标记
'   组状态: MODBUS_REG(REG_TRACE_CMD_STATE_BEGIN + 组号)
'     F_DataUpdate=1 上位机已写入, F_DataUsed=2 控制器已消费
' 由 FSM 在 SYS_ROBOT_MODE 下通过 RUNTASK TASK_TRAJ, TRAJ_MOVE() 启动，
' 停止/急停由 FSM 通过 STOPTASK + RAPIDSTOP 处理，本函数无需关心。
' ================================================================
GLOBAL SUB TRAJ_MOVE()
    PRINT "Traj move start."

    LOCAL n_loop          ' 已消费的组计数
    LOCAL cur_group_id    ' 当前缓冲数据组号
    LOCAL data_state      ' 当前组数据状态
    LOCAL i_loop          ' 指令循环变量(TABLE索引)
    LOCAL cmd_id          ' 指令代号
    LOCAL loop_start_index
    LOCAL loop_end_index
    LOCAL traj_done       ' 轨迹结束标志

    ' 切到虚拟轴（机器人模式下 CONNFRAME 已由 FSM 建立）
    BASE(6,7,8,9,10)
    SPEED = JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED
    ACCEL = JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC
    DECEL = JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC

    traj_done = 0
    n_loop = 0
    WHILE (traj_done = 0)
        cur_group_id = n_loop MOD NUM_TRACE_DATA_GROUP

        ' 等待上位机写入当前组数据
        data_state = MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group_id)
        WHILE (data_state <> F_DataUpdate)
            DELAY(10)
            data_state = MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group_id)
        WEND

        loop_start_index = TABLE_TRAJ_BEGIN + cur_group_id * SIZE_TRAJ_BLOCK
        loop_end_index = loop_start_index + SIZE_TRAJ_BLOCK - 1

        ' 逐条下发指令，不做 WAIT IDLE，依靠 MERGE 连续插补
        i_loop = loop_start_index
        WHILE (i_loop <= loop_end_index) AND (traj_done = 0)
            cmd_id = TABLE(i_loop)
            IF cmd_id = CMD_NONE THEN
                ' 轨迹结束标记（TABLE未写区域默认为0）
                traj_done = 1
            ELSEIF cmd_id = CMD_MOVE THEN
                MOVE(TABLE(i_loop+1), TABLE(i_loop+2), TABLE(i_loop+3), TABLE(i_loop+4), TABLE(i_loop+5))
            ELSEIF cmd_id = CMD_MOVE_ABS THEN
                MOVEABS(TABLE(i_loop+1), TABLE(i_loop+2), TABLE(i_loop+3), TABLE(i_loop+4), TABLE(i_loop+5))
            ELSEIF cmd_id = CMD_MOVE_PT THEN
                MOVE_PT(TABLE(i_loop+6), TABLE(i_loop+1), TABLE(i_loop+2), TABLE(i_loop+3), TABLE(i_loop+4), TABLE(i_loop+5))
            ELSEIF cmd_id = CMD_MOVE_PTABS THEN
                MOVE_PTABS(TABLE(i_loop+6), TABLE(i_loop+1), TABLE(i_loop+2), TABLE(i_loop+3), TABLE(i_loop+4), TABLE(i_loop+5))
            ELSEIF cmd_id = CMD_MOVE_DELAY THEN
                MOVE_DELAY(TABLE(i_loop+6))
            ELSE
                ' 非法指令：取消缓冲运动并上报错误
                PRINT "[TRAJ] 非法指令代号: ", cmd_id, " TABLE索引: ", i_loop
                RAPIDSTOP(2)
                MODBUS_REG(REG_CMD_ERROR) = ERR_UNKNOWN_CMD
                traj_done = 1
            ENDIF
            i_loop = i_loop + SIZE_TRAJ_CMD
        WEND

        ' 当前组已消费，通知上位机可重写
        MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group_id) = F_DataUsed
        n_loop = n_loop + 1
    WEND

    ' 等待最后一段运动完成后通知FSM
    WAIT IDLE
    PRINT "Traj move over."
    MODBUS_REG(REG_EVENT_L1) = EVENT_TRAJ_DONE
END SUB
