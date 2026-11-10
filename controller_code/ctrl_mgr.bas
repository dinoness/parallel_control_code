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
' 5. 力传感器硬件适配层通过 TABLE 400-419 接入，默认关闭；未接入硬件、
'    数据断流或过力时禁止进入/继续力控。当前仅对 Z 施加增量式 PI 修正。
' ================================================================

' 初始化状态采样、力控默认参数，并注册全工程唯一的伺服周期中断
GLOBAL SUB STATUS_INIT()
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
    FOR i = 0 TO 4
        nominal_pos(i) = 0
        adj_pos(i) = 0
    NEXT
    FOR i = 0 TO 5
        force_raw(i) = 0
    NEXT

    ' 默认安全参数：传感器关闭、PI 增益为 0。接入硬件后再修改比例和零位。
    TABLE(TABLE_FORCE_CFG_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0, 0.0, 1.0, 0.0, 0.0, 50.0, 5000.0, 1000.0, 5, 1.0, 10)

    TABLE(TABLE_STATUS_BASE) = 0
    TABLE(TABLE_STATUS_BASE + 1) = 0
    INT_CYCLE(1, TASK_CTRL, SERVO_CYCLE)
END SUB

' 统一伺服周期入口：采集和控制优先，状态采样常驻
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

' 力传感器采集适配：硬件层更新采样序号和原始通道，本函数完成断流检测、标定和滤波
GLOBAL SUB FORCE_SAMPLE()
    LOCAL i, sample_count, alpha

    IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_ENABLE) = 0 THEN
        force_sensor_status = FORCE_SENSOR_DISABLED
        force_stale_ticks = 0
    ELSEIF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_VALID) = 0 THEN
        force_sensor_status = FORCE_SENSOR_INVALID
        force_stale_ticks = 0
    ELSE
        sample_count = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_SAMPLE_COUNT)
        IF sample_count = force_last_sample_count THEN
            force_stale_ticks = force_stale_ticks + 1
        ELSE
            force_last_sample_count = sample_count
            force_stale_ticks = 0
        ENDIF

        IF force_stale_ticks > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_STALE_TICKS) THEN
            force_sensor_status = FORCE_SENSOR_STALE
        ELSE
            FOR i = 0 TO 5
                force_raw(i) = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_RAW_BEGIN + i)
            NEXT
            force_normal = (force_raw(2) - TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_ZERO)) * TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_SCALE)
            alpha = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_FILTER_ALPHA)
            IF alpha <= 0 OR alpha > 1 THEN
                alpha = 1
            ENDIF
            force_filtered = force_filtered + alpha * (force_normal - force_filtered)
            force_sensor_status = FORCE_SENSOR_VALID
            IF ctrl_running = 1 AND ABS(force_filtered) > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_FORCE) THEN
                force_sensor_status = FORCE_SENSOR_OVERLIMIT
            ENDIF
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

' 状态采样：100Hz 提交一帧，32 通道分 10 个伺服周期写入。
' 每周期最多处理 4 个数据通道，避免与 CTRL 运动下发叠加后超过 333us。
' header 始终最后更新，上位机只会把已提交帧视为完整帧。
GLOBAL SUB STATUS_SAMPLE()
    LOCAL base_addr

    base_addr = TABLE_STATUS_BASE + STATUS_HEADER_FLOATS + status_write_index * STATUS_CHANNELS

    IF status_tick = 0 THEN
        TABLE(base_addr) = MPOS(0) - dl_base(0)
        TABLE(base_addr + 1) = MPOS(1) - dl_base(1)
        TABLE(base_addr + 2) = MPOS(2) - dl_base(2)
    ELSEIF status_tick = 1 THEN
        TABLE(base_addr + 3) = MPOS(3) - dl_base(3)
        TABLE(base_addr + 4) = MPOS(4) - dl_base(4)
        TABLE(base_addr + 5) = ENCODER(0)
    ELSEIF status_tick = 2 THEN
        TABLE(base_addr + 6) = ENCODER(1)
        TABLE(base_addr + 7) = ENCODER(2)
        TABLE(base_addr + 8) = ENCODER(3)
        TABLE(base_addr + 9) = ENCODER(4)
    ELSEIF status_tick = 3 THEN
        TABLE(base_addr + 10) = DRIVE_TORQUE(0)
        TABLE(base_addr + 11) = DRIVE_TORQUE(1)
        TABLE(base_addr + 12) = DRIVE_TORQUE(2)
    ELSEIF status_tick = 4 THEN
        TABLE(base_addr + 13) = DRIVE_TORQUE(3)
        TABLE(base_addr + 14) = DRIVE_TORQUE(4)
        IF ee_valid = 1 THEN
            TABLE(base_addr + 15) = DPOS(6)
        ENDIF
    ELSEIF status_tick = 5 THEN
        IF ee_valid = 1 THEN
            TABLE(base_addr + 16) = DPOS(7)
            TABLE(base_addr + 17) = DPOS(8)
            TABLE(base_addr + 18) = DPOS(9)
        ENDIF
    ELSEIF status_tick = 6 THEN
        IF ee_valid = 1 THEN
            TABLE(base_addr + 19) = DPOS(10)
            TABLE(base_addr + 20) = 1
        ELSE
            TABLE(base_addr + 20) = 0
        ENDIF
        TABLE(base_addr + 21) = force_raw(0)
    ELSEIF status_tick = 7 THEN
        TABLE(base_addr + 22) = force_raw(1)
        TABLE(base_addr + 23) = force_raw(2)
        TABLE(base_addr + 24) = force_raw(3)
        TABLE(base_addr + 25) = force_raw(4)
    ELSEIF status_tick = 8 THEN
        TABLE(base_addr + 26) = force_raw(5)
        TABLE(base_addr + 27) = force_filtered
        TABLE(base_addr + 28) = force_target_effective
        TABLE(base_addr + 29) = adj_pos(2)
    ELSE
        TABLE(base_addr + 30) = force_state
        TABLE(base_addr + 31) = force_sensor_status
        ' 第 10 段：写完最后两个通道后立即提交完整帧
        status_write_index = (status_write_index + 1) MOD STATUS_RING_FRAMES
        status_frame_counter = status_frame_counter + 1
        TABLE(TABLE_STATUS_BASE, status_frame_counter, status_write_index)
    ENDIF

    status_tick = status_tick + 1
    IF status_tick >= STATUS_SAMPLE_DIV THEN
        status_tick = 0
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
            IF force_sensor_status <> FORCE_SENSOR_VALID THEN
                CTRL_ABORT(ERR_FORCE_SENSOR_INVALID)
                RETURN
            ENDIF
            IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_CONTACT_TICKS) < 1 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_FORCE) <= 0 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST) <= 0 OR TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJ_RATE) <= 0 THEN
                CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                RETURN
            ENDIF
            IF TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) <> 1 AND TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) <> -1 THEN
                CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                RETURN
            ENDIF
            FOR i = 0 TO 4
                nominal_pos(i) = DPOS(6 + i)
                adj_pos(i) = 0
            NEXT
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
            FOR i = 0 TO 4
                adj_pos(i) = 0
            NEXT
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

    ' 力控计算每个周期执行，即使本周期没有新 PT 点也更新保持目标
    IF force_active = 1 THEN
        IF force_sensor_status = FORCE_SENSOR_OVERLIMIT THEN
            CTRL_ABORT(ERR_FORCE_OVERLIMIT)
            RETURN
        ELSEIF force_sensor_status <> FORCE_SENSOR_VALID THEN
            CTRL_ABORT(ERR_FORCE_SENSOR_INVALID)
            RETURN
        ENDIF

        dt_sec = SERVO_PERIOD / 1000000.0
        IF force_state = FORCE_MODE_SEARCH THEN
            IF ABS(force_filtered) >= force_contact_threshold THEN
                force_contact_count = force_contact_count + 1
            ELSE
                force_contact_count = 0
            ENDIF
            IF force_contact_count >= TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_CONTACT_TICKS) THEN
                force_state = FORCE_MODE_HOLD
            ENDIF
        ENDIF

        IF force_state = FORCE_MODE_HOLD THEN
            target_step = force_ramp_rate * dt_sec
            IF force_target_effective < force_target_cmd THEN
                force_target_effective = force_target_effective + target_step
                IF force_target_effective > force_target_cmd THEN
                    force_target_effective = force_target_cmd
                ENDIF
            ELSEIF force_target_effective > force_target_cmd THEN
                force_target_effective = force_target_effective - target_step
                IF force_target_effective < force_target_cmd THEN
                    force_target_effective = force_target_cmd
                ENDIF
            ENDIF

            force_error = force_target_effective - force_filtered
            adj_step = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_KP) * (force_error - force_prev_error) + TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_KI) * force_error * dt_sec
            max_step = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJ_RATE) * dt_sec
            IF adj_step > max_step THEN
                adj_step = max_step
            ELSEIF adj_step < -max_step THEN
                adj_step = -max_step
            ENDIF
            adj_pos(2) = adj_pos(2) + TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_DIRECTION) * adj_step
            IF adj_pos(2) > force_adjust_limit THEN
                adj_pos(2) = force_adjust_limit
            ELSEIF adj_pos(2) < -force_adjust_limit THEN
                adj_pos(2) = -force_adjust_limit
            ENDIF
            force_prev_error = force_error
        ELSE
            force_prev_error = 0
        ENDIF
    ENDIF

    ' 运动指令在缓冲满时可能阻塞，保留至少两个缓冲位置后再读取和下发
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
            IF cmd_id = CMD_NONE THEN
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                traj_over = 1
                has_target = 0
                exec_state = EXEC_WAIT_ALL_IDLE
                consume_cmd = 0
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
                        exec_state = EXEC_WAIT_NORMAL_IDLE
                    ENDIF
                ELSE
                    CTRL_ABORT(ERR_FORCE_PARAM_INVALID)
                ENDIF
            ELSEIF cmd_id = CMD_FORCE_OFF THEN
                ' 普通段中的 FORCE_OFF 是无操作，便于轨迹模板复用
            ELSE
                CTRL_ABORT(ERR_UNKNOWN_CMD)
            ENDIF

        ELSEIF exec_state = EXEC_FORCE THEN
            IF cmd_id = CMD_MOVE_PTABS THEN
                FOR i = 0 TO 4
                    nominal_pos(i) = TABLE(ctrl_index + 1 + i)
                NEXT
                has_target = 1
            ELSEIF cmd_id = CMD_MOVE_PT THEN
                FOR i = 0 TO 4
                    nominal_pos(i) = nominal_pos(i) + TABLE(ctrl_index + 1 + i)
                NEXT
                has_target = 1
            ELSEIF cmd_id = CMD_FORCE_CFG THEN
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
                        IF force_adjust_limit > TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST) THEN
                            force_adjust_limit = TABLE(TABLE_FORCE_CFG_BEGIN + FORCE_CFG_MAX_ADJUST)
                        ENDIF
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
        IF consume_cmd = 1 AND ctrl_running = 1 THEN
            ctrl_index = ctrl_index + SIZE_TRAJ_CMD
            IF group_offset + SIZE_TRAJ_CMD >= SIZE_TRAJ_BLOCK THEN
                MODBUS_REG(REG_TRACE_CMD_STATE_ST + cur_group) = F_DataUsed
                ctrl_index = TABLE_TRAJ_BEGIN + ((cur_group + 1) MOD NUM_TRACE_DATA_GROUP) * SIZE_TRAJ_BLOCK
            ENDIF
        ENDIF
    ENDIF

    ' 无新点时仍按最新力误差输出保持点；FORCE_OFF 后不再发送零修正 PT
    IF ctrl_running = 1 AND exec_state = EXEC_FORCE AND force_active = 1 AND has_target = 1 THEN
        MOVE_PTABS(1, nominal_pos(0)+adj_pos(0), nominal_pos(1)+adj_pos(1), nominal_pos(2)+adj_pos(2), nominal_pos(3)+adj_pos(3), nominal_pos(4)+adj_pos(4))
    ENDIF
END SUB
