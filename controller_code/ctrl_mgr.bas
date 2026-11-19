' ================================================================
' ctrl_mgr.bas - 统一伺服采样、CTRL 混合轨迹与法向力控
'
' 1. 全工程仅 TASK_CTRL 注册一个 INT_CYCLE。SERVO_CYCLE 每周期依次执行：
'    力传感器采集 -> CTRL 混合轨迹步进 -> 状态采样上传。
' 2. TRAJ_MOVE 仍负责普通 Trace；EVENT_CTRL 启动本文件的混合轨迹执行器。
' 3. 普通段支持 MOVE/MOVEABS/MOVE_PT/MOVE_PTABS/MOVE_DELAY。
'    CMD_FORCE_CFG 后等待普通运动完成，再进入力控段；力控段仅接受 PT 指令、
'    FORCE_CFG 和 FORCE_OFF。CMD_NONE 前必须显式 FORCE_OFF。
' 4. 力控段内部统一维护 nominal_pos，最终使用 ticks=1 的 MOVE_PTABS 输出
'    nominal_pos + adj_pos，避免相对 PT 重复累加修正量。
' 5. EL3702 通过标准 AIN 和 TABLE 400-419 适配，第一阶段仅采集显示且 PI 增益为0；
'    未接入硬件、数据断流或过力时禁止进入/继续力控。当前仅对 Z 施加增量式 PI 修正。
' ================================================================

' 初始化状态采样、力控默认参数，并注册全工程唯一的伺服周期中断
GLOBAL SUB STATUS_INIT()
    DEFINE_CFUNC cfunc_impendance FLOAT impendanceCtrl(FLOAT f_sensor, FLOAT f_ref)


    LOCAL i
    PRINT "统一状态采样与 CTRL 混合轨迹任务启动"
    DL_BASE_CAPTURE()
    ee_valid = 0
    status_frame_counter = 0
    status_write_index = 0
    status_tick = 0

    ctrl_running = 0
    ctrl_paused = 0
    traj_over = 0
    exec_state = EXEC_NORMAL
    force_active = 0
    force_state = FORCE_MODE_IDLE
    force_cfg_mode = FORCE_MODE_IDLE
    force_target_cmd = 0
    force_target_effective = 0
    force_contact_threshold = 0
    force_ramp_rate = 0
    force_adjust_limit = 0
    force_contact_count = 0
    force_prev_error = 0
    buffer_wait_ticks = 0
    has_target = 0

    force_normal = 0
    force_filtered = 0
    force_sensor_status = FORCE_SENSOR_DISABLED
    force_last_sample_count = -1
    force_stale_ticks = 0
    force_hw_valid = 0
    force_enabled = 0
    FOR i = 0 TO 4
        nominal_pos(i) = 0
        adj_pos(i) = 0
    NEXT
    FOR i = 0 TO 5
        force_raw(i) = 0
    NEXT

    ' 第一阶段只采集和显示：启用硬件适配，PI增益保持0；比例是理论初值，需实测校准。
    ' 末位断流阈值单位为 10ms（断流检测已降频到 10ms 低频维护段执行）
    TABLE(TABLE_FORCE_CFG_BEGIN, 1, 0, 0, 0, 0, 0, 0, 0, 0, FORCE_FZ_DEFAULT_SCALE, 0.0, 1.0, 0.0, 0.0, 50.0, 5000.0, 1000.0, 5, 1.0, 1)

    TABLE(TABLE_STATUS_BASE) = 0
    TABLE(TABLE_STATUS_BASE + 1) = 0
    force_stream_count = 0
    force_stream_write_index = 0
    TABLE(TABLE_FORCE_STREAM_BASE, 0, 0)
    INT_CYCLE(1, TASK_CTRL, SERVO_CYCLE)
END SUB

' 统一伺服周期入口：力传感器采集与适配优先，状态采样常驻
GLOBAL SUB SERVO_CYCLE()
    FORCE_SAMPLE()
    IF ctrl_running = 1 AND ctrl_paused = 0 THEN
        CTRL_TRACE_STEP()
    ENDIF
    STATUS_SAMPLE()
END SUB

' 标定 dL 零位基准：回零完成后调用
GLOBAL SUB DL_BASE_CAPTURE()
    LOCAL i
    FOR i = 0 TO 4
        dl_base(i) = MPOS(i)
    NEXT
END SUB

' 力传感器采集与适配（单函数，分高频热路径和 10ms 低频维护两段）。
' 热路径（每伺服周期）：AIN(33) 直读 Fz 原始值 -> 无分支补码转换 -> 标定滤波一条语句 -> 过力判断。
' 低频维护（借用 STATUS_SAMPLE 的分频计数，status_tick=5 触发，错开 tick=9 的状态帧写周期）：
'   NODE_STATUS、ENABLE 读取、断流检测、状态细分、TABLE(401/402/405) 调试镜像。
' 总线周期 1ms 与伺服周期 1:1，每周期必有新记录，不再读 CycleCounter。
' 参数保留在 TABLE 400-419 以便上位机/在线调参，不用 GLOBAL CONST。
GLOBAL SUB FORCE_SAMPLE()
    LOCAL fz_raw

    ' ---- 10ms 低频维护 ----
    IF status_tick = 5 THEN
        ' 节点在线状态，10ms 响应
        IF NODE_STATUS(BUS_SLOT, FORCE_NODE) = 1 THEN
            force_hw_valid = 1
        ELSE
            force_hw_valid = 0
        ENDIF
        ' 适配启停参数，10ms 响应
        IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_ENABLE) = 0 THEN
            force_enabled = 0
        ELSE
            force_enabled = 1
        ENDIF
        ' 断流检测：10ms 内采样序号未前进则累计一拍；TABLE(419) 阈值单位为 10ms
        IF force_hw_sample_count = force_last_sample_count THEN
            force_stale_ticks = force_stale_ticks + 1
        ELSE
            force_last_sample_count = force_hw_sample_count
            force_stale_ticks = 0
        ENDIF
        ' 调试镜像：仅供 ZDevelop 观察，控制路径不依赖这些 TABLE
        TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_VALID) = force_hw_valid
        TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_SAMPLE_COUNT) = force_hw_sample_count
        TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_RAW_BEGIN + 2) = force_raw(2)
        ' 状态细分；OVERLIMIT 由热路径置位且随即触发 CTRL_ABORT，此处覆盖回 VALID 无控制影响
        IF force_enabled = 0 THEN
            force_sensor_status = FORCE_SENSOR_DISABLED
        ELSEIF force_hw_valid = 0 THEN
            force_sensor_status = FORCE_SENSOR_INVALID
        ELSEIF force_stale_ticks > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_STALE_TICKS) THEN
            force_sensor_status = FORCE_SENSOR_STALE
        ELSE
            force_sensor_status = FORCE_SENSOR_VALID
        ENDIF
    ENDIF

    ' ---- 热路径：每伺服周期执行 ----
    IF force_enabled = 1 AND force_hw_valid = 1 THEN
        ' 无分支补码转换：AIN 无论按有符号还是无符号返回 16 位原始码都正确
        fz_raw = (AIN(FORCE_AIN_BASE + 1) + 32768) MOD 65536 - 32768
        force_raw(2) = fz_raw
        force_hw_sample_count = force_hw_sample_count + 1

        ' ' 独立高频流：每伺服周期保存1帧，先写数据再提交header。
        ' TABLE(FORCE_STREAM_DATA_BEGIN + force_stream_write_index) = fz_raw
        ' force_stream_write_index = (force_stream_write_index + 1) MOD FORCE_STREAM_FRAMES
        ' force_stream_count = force_stream_count + 1
        ' TABLE(TABLE_FORCE_STREAM_BASE, force_stream_count, force_stream_write_index)

        ' 标定 + 一阶低通合并为一条；alpha 的合法范围 (0,1] 由参数配置侧保证
        force_filtered = force_filtered + TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_FILTER_ALPHA) * ((fz_raw - TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_ZERO)) * TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_SCALE) - force_filtered)
        ' 传感器启用时，过力保护覆盖 CTRL 的普通段和力控段
        IF ctrl_running = 1 AND ABS(force_filtered) > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_FORCE) THEN
            force_sensor_status = FORCE_SENSOR_OVERLIMIT
        ENDIF
    ENDIF
END SUB

' 空载去皮：只能在未接触且 CTRL 未运行时调用；硬件原始值必须先有效
GLOBAL SUB FORCE_TARE()
    IF force_sensor_status = FORCE_SENSOR_VALID AND ctrl_running = 0 THEN
        TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_ZERO) = force_raw(2)
        force_normal = 0
        force_filtered = 0
        force_prev_error = 0
        PRINT "力传感器 Fz 零位已更新"
    ELSE
        PRINT "拒绝去皮：请确认传感器有效、工具未接触且 CTRL 已停止"
    ENDIF
END SUB

' 状态采样：每 10ms 在一个伺服周期内用一条 TABLE 指令提交完整 22 通道帧(100Hz)。
' 布局：[0-4] dL；[5-9] 扭矩；[10-14] 末端位姿；[15] ee_valid；[16-21] 六轴力原始值。
' 不上传编码器及滤波力、目标力、修正量等派生状态；header 最后更新以保证帧完整。
GLOBAL SUB STATUS_SAMPLE()
    LOCAL base_addr

    status_tick = status_tick + 1
    IF status_tick >= STATUS_SAMPLE_DIV THEN
        status_tick = 0
        base_addr = TABLE_STATUS_BASE + STATUS_HEADER_FLOATS + status_write_index * STATUS_CHANNELS

        IF ee_valid = 1 THEN
            TABLE(base_addr, MPOS(0)-dl_base(0), MPOS(1)-dl_base(1), MPOS(2)-dl_base(2), MPOS(3)-dl_base(3), MPOS(4)-dl_base(4), DRIVE_TORQUE(0), DRIVE_TORQUE(1), DRIVE_TORQUE(2), DRIVE_TORQUE(3), DRIVE_TORQUE(4), DPOS(6), DPOS(7), DPOS(8), DPOS(9), DPOS(10), 1, force_raw(0), force_raw(1), force_raw(2), force_raw(3), force_raw(4), force_raw(5))
        ELSE
            TABLE(base_addr, MPOS(0)-dl_base(0), MPOS(1)-dl_base(1), MPOS(2)-dl_base(2), MPOS(3)-dl_base(3), MPOS(4)-dl_base(4), DRIVE_TORQUE(0), DRIVE_TORQUE(1), DRIVE_TORQUE(2), DRIVE_TORQUE(3), DRIVE_TORQUE(4), 0, 0, 0, 0, 0, 0, force_raw(0), force_raw(1), force_raw(2), force_raw(3), force_raw(4), force_raw(5))
        ENDIF

        ' 完整帧写完后再提交 header
        status_write_index = (status_write_index + 1) MOD STATUS_RING_FRAMES
        status_frame_counter = status_frame_counter + 1
        TABLE(TABLE_STATUS_BASE, status_frame_counter, status_write_index)
    ENDIF
END SUB

' 启动完整 CTRL 混合轨迹，而不是只启动力控段
GLOBAL SUB CTRL_MOVE()
    LOCAL i
    PRINT "CTRL 混合轨迹启动"
    has_target = 0
    traj_over = 0
    ctrl_paused = 0
    ctrl_index = TABLE_TRAJ_BEGIN
    exec_state = EXEC_NORMAL
    force_active = 0
    force_state = FORCE_MODE_IDLE
    force_cfg_mode = FORCE_MODE_IDLE
    force_target_cmd = 0
    force_target_effective = 0
    force_contact_count = 0
    force_prev_error = 0
    buffer_wait_ticks = 0
    FOR i = 0 TO 4
        adj_pos(i) = 0
    NEXT

    BASE(6,7,8,9,10)
    SPEED = JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED, JOINT_L2_SPEED
    ACCEL = JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC
    DECEL = JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC, JOINT_L2_ACC
    MODBUS_REG(REG_CMD_ERROR) = ERR_OK
    ctrl_running = 1
END SUB

' 停止混合轨迹；急停/停止由 FSM 继续执行 RAPIDSTOP
GLOBAL SUB CTRL_MOVE_END()
    LOCAL i
    ctrl_running = 0
    ctrl_paused = 0
    has_target = 0
    force_active = 0
    force_state = FORCE_MODE_IDLE
    force_target_effective = 0
    exec_state = EXEC_NORMAL
    FOR i = 0 TO 4
        adj_pos(i) = 0
    NEXT
    PRINT "CTRL 混合轨迹停止"
END SUB

' 混合轨迹异常：立即停止并清空运动，FSM 根据错误码进入 SYS_ERROR
GLOBAL SUB CTRL_ABORT(error_code)
    BASE(6,7,8,9,10)
    RAPIDSTOP(2)
    MODBUS_REG(REG_CMD_ERROR) = error_code
    traj_over = 1
    CTRL_MOVE_END()
    force_state = FORCE_MODE_FAULT
    MODBUS_REG(REG_EVENT_L1) = EVENT_CTRL_DONE
    PRINT "[CTRL] 混合轨迹异常停止，错误码: ", error_code
END SUB

' 暂停当前运动及后续指令消费，TASK_CTRL 和状态采样保持运行
GLOBAL SUB CTRL_MOVE_PAUSE()
    IF ctrl_running = 1 AND ctrl_paused = 0 THEN
        ctrl_paused = 1
        BASE(6,7,8,9,10)
        MOVE_PAUSE(3)
    ENDIF
END SUB

' 从暂停位置恢复，保留轨迹索引和力控状态
GLOBAL SUB CTRL_MOVE_RESUME()
    IF ctrl_running = 1 AND ctrl_paused = 1 THEN
        BASE(6,7,8,9,10)
        MOVE_RESUME
        force_prev_error = 0
        ctrl_paused = 0
    ENDIF
END SUB

' 每伺服周期执行一次的非阻塞混合轨迹状态机
GLOBAL SUB CTRL_TRACE_STEP()
    LOCAL i, cur_group, group_offset, cmd_id, can_read, consume_cmd
    LOCAL dt_sec, force_error, adj_step, max_step, target_step, new_mode

    BASE(6,7,8,9,10)

    ' 传感器启用时，过力保护覆盖 CTRL 的普通段和力控段
    IF force_sensor_status = FORCE_SENSOR_OVERLIMIT THEN
        CTRL_ABORT(ERR_FORCE_OVERLIMIT)
        RETURN
    ENDIF

    ' 阶段等待只查询 IDLE，不在周期中断中阻塞
    IF exec_state = EXEC_WAIT_NORMAL_IDLE THEN
        IF IDLE(6) AND IDLE(7) AND IDLE(8) AND IDLE(9) AND IDLE(10) THEN
            ' IF force_sensor_status <> FORCE_SENSOR_VALID THEN
            '     CTRL_ABORT(ERR_FORCE_SENSOR_INVALID)
            '     RETURN
            ' ENDIF
            ' ' 这些条件都是什么意思：不超时，不过量程，不调整过大，控制变化率===========================================================
            ' IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_CONTACT_TICKS) < 1 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_FORCE) <= 0 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST) <= 0 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJ_RATE) <= 0 THEN
            '     CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
            '     RETURN
            ' ENDIF
            ' ' z修正方向是hyw（估计是用了绝对值+符号）================================================================================
            ' IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) <> 1 AND TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) <> -1 THEN
            '     CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
            '     RETURN
            ' ENDIF

            nominal_pos(0, DPOS(6), DPOS(7), DPOS(8), DPOS(9),  DPOS(10))
            adj_pos(0, 0, 0, 0, 0, 0)
            ' 搞懂每个变量的含义==========================================================================================
            has_target = 0
            force_contact_count = 0
            force_prev_error = 0
            force_target_effective = 0
            force_state = force_cfg_mode
            force_active = 1
            exec_state = EXEC_FORCE
        ELSE
            RETURN
        ENDIF
    ELSEIF exec_state = EXEC_WAIT_FORCE_IDLE THEN
        IF IDLE(6) AND IDLE(7) AND IDLE(8) AND IDLE(9) AND IDLE(10) THEN
            adj_pos(0, 0, 0, 0, 0, 0)
            force_state = FORCE_MODE_IDLE
            force_target_effective = 0
            force_prev_error = 0
            exec_state = EXEC_NORMAL
        ELSE
            RETURN
        ENDIF
    ELSEIF exec_state = EXEC_WAIT_ALL_IDLE THEN
        IF IDLE(6) AND IDLE(7) AND IDLE(8) AND IDLE(9) AND IDLE(10) THEN
            CTRL_MOVE_END()
            MODBUS_REG(REG_EVENT_L1) = EVENT_CTRL_DONE
        ENDIF
        RETURN
    ENDIF

    ' 力控计算每个周期执行，即使本周期没有新PT点也更新保持目标
    ' force_active是否和exec_force等价？==========================================================================================
    IF force_active = 1 THEN
        IF force_sensor_status <> FORCE_SENSOR_VALID THEN
            CTRL_ABORT(ERR_FORCE_SENSOR_INVALID)
            RETURN
        ENDIF

        ' dt_sec = SERVO_PERIOD / 1000000.0
        ' IF force_state = FORCE_MODE_SEARCH THEN
        '     IF ABS(force_filtered) >= force_contact_threshold THEN
        '         force_contact_count = force_contact_count + 1
        '     ELSE
        '         force_contact_count = 0
        '     ENDIF
        '     IF force_contact_count >= TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_CONTACT_TICKS) THEN
        '         force_state = FORCE_MODE_HOLD
        '     ENDIF
        ' ENDIF

        ' 这段是基于力误差，计算调整量=============
        IF force_state = FORCE_MODE_HOLD THEN
            ' target_step = force_ramp_rate * dt_sec
            ' IF force_target_effective < force_target_cmd THEN
            '     force_target_effective = force_target_effective + target_step
            '     IF force_target_effective > force_target_cmd THEN
            '         force_target_effective = force_target_cmd
            '     ENDIF
            ' ELSEIF force_target_effective > force_target_cmd THEN
            '     force_target_effective = force_target_effective - target_step
            '     IF force_target_effective < force_target_cmd THEN
            '         force_target_effective = force_target_cmd
            '     ENDIF
            ' ENDIF

            ' force_error = force_target_effective - force_filtered
            ' adj_step = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_KP) * (force_error - force_prev_error) + TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_KI) * force_error * dt_sec
            ' max_step = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJ_RATE) * dt_sec
            ' IF adj_step > max_step THEN
            '     adj_step = max_step
            ' ELSEIF adj_step < -max_step THEN
            '     adj_step = -max_step
            ' ENDIF
            ' adj_pos(2) = adj_pos(2) + TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) * adj_step
            ' IF adj_pos(2) > force_adjust_limit THEN
            '     adj_pos(2) = force_adjust_limit
            ' ELSEIF adj_pos(2) < -force_adjust_limit THEN
            '     adj_pos(2) = -force_adjust_limit
            ' ENDIF
            ' force_prev_error = force_error
            adj_pos(2) = cfunc_impendance(force_filtered, force_target_cmd)
        ELSE
            force_prev_error = 0
        ENDIF
    ENDIF

    ' 运动指令在缓冲满时会在中断内阻塞并直接导致 1034 超时，必须保留至少两个缓冲位置后再读取和下发
    ' REMAIN_BUFFER 的括号参数是运动类型而不是轴号；省略类型并显式指定虚拟主轴 6
    IF REMAIN_BUFFER AXIS(6) <= 2 THEN
        IF force_active = 1 THEN
            buffer_wait_ticks = buffer_wait_ticks + 1
            IF buffer_wait_ticks > 100 THEN
                CTRL_ABORT(ERR_FORCE_BUFFER_TIMEOUT)
            ENDIF
        ENDIF
        RETURN
    ENDIF

    ' 每周期最多读取一条指令；新组未就绪时，力控段保持上一名义目标
    can_read = 0
    group_offset = (ctrl_index - TABLE_TRAJ_BEGIN) MOD SIZE_TRAJ_BLOCK
    cur_group = (ctrl_index - TABLE_TRAJ_BEGIN) \ SIZE_TRAJ_BLOCK
    ' group_offset是什么意思？为什么ELSE后can_read=1===============================================================
    IF group_offset = 0 THEN
        IF MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUpdate THEN
            can_read = 1
        ELSEIF force_active = 1 THEN
            buffer_wait_ticks = buffer_wait_ticks + 1
            IF buffer_wait_ticks > 100 THEN
                CTRL_ABORT(ERR_FORCE_BUFFER_TIMEOUT)
                RETURN
            ENDIF
        ENDIF
    ELSE
        can_read = 1
    ENDIF

    consume_cmd = 0
    IF can_read = 1 THEN
        buffer_wait_ticks = 0
        cmd_id = TABLE(ctrl_index)
        consume_cmd = 1

        IF exec_state = EXEC_NORMAL THEN
            ' 调整指令的判定顺序==========================================================================
            IF cmd_id = CMD_NONE THEN
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                traj_over = 1
                has_target = 0
                exec_state = EXEC_WAIT_ALL_IDLE
                consume_cmd = 0
                ' 不直接next?，这样可以少用consume_cmd这个变量==========================================================================================
            ELSEIF cmd_id = CMD_MOVE THEN
                MOVE(TABLE(ctrl_index+1), TABLE(ctrl_index+2), TABLE(ctrl_index+3), TABLE(ctrl_index+4), TABLE(ctrl_index+5))
            ELSEIF cmd_id = CMD_MOVE_ABS THEN
                MOVEABS(TABLE(ctrl_index+1), TABLE(ctrl_index+2), TABLE(ctrl_index+3), TABLE(ctrl_index+4), TABLE(ctrl_index+5))
            ELSEIF cmd_id = CMD_MOVE_PT THEN
                MOVE_PT(TABLE(ctrl_index+6), TABLE(ctrl_index+1), TABLE(ctrl_index+2), TABLE(ctrl_index+3), TABLE(ctrl_index+4), TABLE(ctrl_index+5))
            ELSEIF cmd_id = CMD_MOVE_PTABS THEN
                MOVE_PTABS(TABLE(ctrl_index+6), TABLE(ctrl_index+1), TABLE(ctrl_index+2), TABLE(ctrl_index+3), TABLE(ctrl_index+4), TABLE(ctrl_index+5))
            ELSEIF cmd_id = CMD_MOVE_DELAY THEN
                MOVE_DELAY(TABLE(ctrl_index+6))
            ELSEIF cmd_id = CMD_FORCE_CFG THEN
                ' 需要好好改改，读取力控制指令的状态，太乱了==========================================================================================
                new_mode = TABLE(ctrl_index+1)
                target_step = TABLE(ctrl_index+2)
                max_step = TABLE(ctrl_index+5)
                IF (new_mode = FORCE_MODE_SEARCH OR new_mode = FORCE_MODE_HOLD) AND target_step >= 0 AND TABLE(ctrl_index+4) > 0 AND max_step > 0 THEN
                    IF new_mode = FORCE_MODE_SEARCH AND TABLE(ctrl_index+3) <= 0 THEN
                        CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                    ELSE
                        force_cfg_mode = new_mode
                        force_target_cmd = target_step
                        force_contact_threshold = TABLE(ctrl_index+3)
                        force_ramp_rate = TABLE(ctrl_index+4)
                        force_adjust_limit = max_step
                        IF force_adjust_limit > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST) THEN
                            force_adjust_limit = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST)
                        ENDIF
                        exec_state = EXEC_WAIT_NORMAL_IDLE  '  结合前文，需要保留
                    ENDIF
                ELSE
                    CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                ENDIF
            ELSEIF cmd_id = CMD_FORCE_OFF THEN
                ' 普通段中的 FORCE_OFF 是无操作，便于轨迹模板复用
            ELSE
                CTRL_ABORT(ERR_UNKNOWN_CMD)
            ENDIF

        ' 力控制模式
        ELSEIF exec_state = EXEC_FORCE THEN
            IF cmd_id = CMD_MOVE_PTABS THEN
                nominal_pos(0, TABLE(ctrl_index+1), TABLE(ctrl_index+2), TABLE(ctrl_index+3), TABLE(ctrl_index+4), TABLE(ctrl_index+5))
                has_target = 1
            ELSEIF cmd_id = CMD_MOVE_PT THEN
                nominal_pos(0, nominal_pos(0)+TABLE(ctrl_index+1), nominal_pos(1)+TABLE(ctrl_index+2), nominal_pos(2)+TABLE(ctrl_index+3), nominal_pos(3)+TABLE(ctrl_index+4), nominal_pos(4)+TABLE(ctrl_index+5))
                has_target = 1
            ELSEIF cmd_id = CMD_FORCE_CFG THEN
                ' 可简化==========================================================================================
                new_mode = TABLE(ctrl_index+1)
                target_step = TABLE(ctrl_index+2)
                max_step = TABLE(ctrl_index+5)
                IF (new_mode = FORCE_MODE_SEARCH OR new_mode = FORCE_MODE_HOLD) AND target_step >= 0 AND TABLE(ctrl_index+4) > 0 AND max_step > 0 THEN
                    IF new_mode = FORCE_MODE_SEARCH AND TABLE(ctrl_index+3) <= 0 THEN
                        CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                    ELSE
                        force_cfg_mode = new_mode
                        force_state = new_mode
                        force_target_cmd = target_step
                        force_contact_threshold = TABLE(ctrl_index+3)
                        force_ramp_rate = TABLE(ctrl_index+4)
                        force_adjust_limit = max_step
                        ' IF force_adjust_limit > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST) THEN
                        '     force_adjust_limit = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST)
                        ' ENDIF
                        force_contact_count = 0
                        force_prev_error = 0
                        IF new_mode = FORCE_MODE_SEARCH THEN
                            force_target_effective = 0
                        ENDIF
                    ENDIF
                ELSE
                    CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                ENDIF
            ELSEIF cmd_id = CMD_FORCE_OFF THEN
                force_active = 0
                has_target = 0
                exec_state = EXEC_WAIT_FORCE_IDLE
            ELSEIF cmd_id = CMD_NONE THEN
                ' 必须先 FORCE_OFF，避免带修正量直接结束
                CTRL_ABORT(ERR_FORCE_CMD_INVALID)
            ELSE
                CTRL_ABORT(ERR_FORCE_CMD_INVALID)
            ENDIF
        ENDIF

        ' 跨组时释放当前组；CMD_NONE 已在上方单独释放且不推进索引
        ' 能否简化？==========================================================================================
        IF consume_cmd = 1 AND ctrl_running = 1 THEN
            ctrl_index = ctrl_index + SIZE_TRAJ_CMD
            IF group_offset + SIZE_TRAJ_CMD >= SIZE_TRAJ_BLOCK THEN
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                ctrl_index = TABLE_TRAJ_BEGIN + ((cur_group + 1) MOD NUM_TRACE_DATA_GROUP) * SIZE_TRAJ_BLOCK
            ENDIF
        ENDIF
    ENDIF

    ' 无新点时仍按最新力误差输出保持点；FORCE_OFF 后不再发送零修正 PT
    ' 能否简化？==========================================================================================
    IF ctrl_running = 1 AND exec_state = EXEC_FORCE AND force_active = 1 AND has_target = 1 THEN
        MOVE_PTABS(1, nominal_pos(0)+adj_pos(0), nominal_pos(1)+adj_pos(1), nominal_pos(2)+adj_pos(2), nominal_pos(3)+adj_pos(3), nominal_pos(4)+adj_pos(4))
    ENDIF
END SUB
