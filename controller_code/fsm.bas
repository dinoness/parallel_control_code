' ================================================================
' fsm.bas - 有限状态机：事件获取、分发、状态切换
' 事件优先级: 寄存器地址越小优先级越高
'   REG_EVENT_BEGIN+0 (90) = 紧急级 (STOP等)
'   REG_EVENT_BEGIN+1 (91) = 中等级 (运动完成等)
'   REG_EVENT_BEGIN+2 (92) = 一般级 (上位机命令)
' ================================================================

' ================================================================
' 事件获取 — 按优先级扫描 MODBUS_REG，返回第一个非 IDLE 事件
' event_register_begin: 事件寄存器起始地址 (REG_EVENT_BEGIN=90)
' event_level_size:    优先级层级数 (MAX_EVENT_LEVEL=3)
' ================================================================
GLOBAL SUB CHECK_EVENT(event_register_begin, event_level_size)
    LOCAL i
    LOCAL new_event
    FOR i = 0 TO (event_level_size - 1) STEP 1
        new_event = MODBUS_REG(i + event_register_begin)
        IF new_event <> EVENT_IDLE THEN
            ' 读取后立即清零，避免重复触发
            MODBUS_REG(i + event_register_begin) = EVENT_IDLE
            RETURN new_event
        ENDIF
    NEXT

    RETURN EVENT_IDLE
END SUB


' ================================================================
' 事件分发 — 根据当前系统状态路由到对应处理器
' ================================================================
GLOBAL SUB SMF_DISPATCH(current_state, cur_event)
    ' 空闲事件：无需处理，直接返回
    IF cur_event = EVENT_IDLE THEN
        RETURN
    ENDIF

    ' 按当前状态分发
    IF current_state = SYS_SERVO_READY THEN
        Handle_SYS_SERVO_READY(cur_event)
    ELSEIF current_state = SYS_HOMING THEN
        Handle_SYS_HOMING(cur_event)
    ELSEIF current_state = SYS_READY THEN
        Handle_SYS_READY(cur_event)
    ELSEIF current_state = SYS_ROBOT_MODE THEN
        Handle_SYS_ROBOT_MODE(cur_event)
    ELSEIF current_state = SYS_RUNNING THEN
        Handle_SYS_RUNNING(cur_event)
    ELSEIF current_state = SYS_PAUSED THEN
        Handle_SYS_PAUSED(cur_event)
    ELSEIF current_state = SYS_ESTOP THEN
        Handle_SYS_ESTOP(cur_event)
    ELSE
        ' SYS_ERROR 及其他未定义状态
        Handle_SYS_ERROR(cur_event)
    ENDIF
END SUB


' ================================================================
' Handle_SYS_SERVO_READY — 伺服就绪但未回零
' 允许: HOME(开始回零), JOINT(允许单轴调整，无需回零)
' 拒绝: CART_JOG, TRAJ (必须先回零获取绝对位姿)
' ================================================================
SUB Handle_SYS_SERVO_READY(cur_event)
    IF cur_event = EVENT_HOME THEN
        ' 启动回零任务
        ' 注: home_robot() 内部使用 DATUM 指令，在后台任务中执行
        '     完成后需通过 MODBUS_REG 写入 EVENT_HOME_DONE 通知FSM
        RUNTASK TASK_HOEM, HOME_ROBOT()
        motion_mode = MODE_HOME
        active_task = TASK_HOEM
        system_state = SYS_HOMING
        PRINT "[FSM] 回零开始，状态: SYS_SERVO_READY -> SYS_HOMING"

    ELSEIF cur_event = EVENT_JOINT THEN
        ' 单轴调整（不需要回零即可执行，关节空间独立控制）
        RUNTASK TASK_JOINT, MANNUAL_JOINT()
        motion_mode = MODE_JOINT_MANUAL
        active_task = TASK_JOINT
        system_state = SYS_RUNNING
        PRINT "[FSM] 单轴调整开始，状态: SYS_SERVO_READY -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_CART_JOG THEN
        ' 拒绝：笛卡尔点动需要先回零（逆解依赖绝对位姿）
        PRINT "[FSM] 拒绝点动: 请先执行回零操作"
        MODBUS_REG(REG_CMD_ERROR) = ERR_NEED_HOME
        ' 状态不变，仍为 SYS_SERVO_READY

    ELSEIF cur_event = EVENT_TRAJ THEN
        ' 拒绝：轨迹执行需要先回零
        PRINT "[FSM] 拒绝轨迹: 请先执行回零操作"
        MODBUS_REG(REG_CMD_ERROR) = ERR_NEED_HOME
        ' 状态不变，仍为 SYS_SERVO_READY

    ELSEIF cur_event = EVENT_CTRL THEN
        ' 拒绝：闭环控制需要先回零
        PRINT "[FSM] 拒绝闭环: 请先执行回零操作"
        MODBUS_REG(REG_CMD_ERROR) = ERR_NEED_HOME
        ' 状态不变，仍为 SYS_SERVO_READY

    ELSEIF cur_event = EVENT_STOP THEN
        ' 当前无运动任务，无需操作
        PRINT "[FSM] 系统已空闲，无需停止"
        ' 状态不变

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停
        ' 调用外部函数: enter_estop() — 立即去使能所有轴
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        PRINT "[FSM] 急停触发，状态: SYS_SERVO_READY -> SYS_ESTOP"

    ELSE
        ' 其他事件在此状态下无意义
        PRINT "[FSM] 系统未回零，等待 HOME 或 JOINT 指令。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_HOMING — 正在回零中
' 允许: STOP(中断回零), HOME_DONE(回零完成)
' ================================================================
SUB Handle_SYS_HOMING(cur_event)
    IF cur_event = EVENT_STOP THEN
        ' 中断回零任务
        STOPTASK TASK_HOEM
        RAPIDSTOP(1)  ' 清楚缓冲的运动
        WAIT IDLE
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 回零被中断，状态: SYS_HOMING -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_HOME_DONE THEN
        ' 回零成功完成
        ' 注: 此事件由 home_robot() 任务完成后写入 MODBUS_REG
        '     home_robot() 需要在函数末尾写入:
        '     MODBUS_REG(REG_EVENT_BEGIN + 1) = EVENT_HOME_DONE
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_READY
        PRINT "[FSM] 回零完成，状态: SYS_HOMING -> SYS_READY"

    ELSEIF cur_event = EVENT_HOME_FAILED THEN
        ' 回零失败（驱动器报警、限位未找到等）
        STOPTASK TASK_HOEM
        RAPIDSTOP(1)
        WAIT IDLE
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_SERVO_READY
        MODBUS_REG(REG_CMD_ERROR) = ERR_HOME_FAILED
        PRINT "[FSM] 回零失败，状态: SYS_HOMING -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_HOME THEN
        ' 已在回零中，拒绝重复请求
        PRINT "[FSM] 回零已在进行中，请等待完成"
        ' 状态不变

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停：中断回零并急停
        STOPTASK TASK_HOEM
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        PRINT "[FSM] 回零中急停，状态: SYS_HOMING -> SYS_ESTOP"

    ELSE
        ' 其他事件在回零期间暂不处理
        PRINT "[FSM] 回零进行中，等待完成。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_READY — 已回零，就绪可进入机器人模式或执行回零/单轴
' 允许: HOME(重新回零), JOINT(单轴), ROBOT_IN(进入机器人模式)
' CART_JOG/TRAJ 需要先进入 SYS_ROBOT_MODE
' ================================================================
SUB Handle_SYS_READY(cur_event)
    IF cur_event = EVENT_HOME THEN
        ' 重新执行回零（例如更换工具后重新标定）
        RUNTASK TASK_HOEM, home_robot()
        motion_mode = MODE_HOME
        active_task = TASK_HOEM
        system_state = SYS_HOMING
        PRINT "[FSM] 重新回零，状态: SYS_READY -> SYS_HOMING"

    ELSEIF cur_event = EVENT_ROBOT_IN THEN
        ' 进入机器人模式，允许笛卡尔空间运动
		ROBOT_MODE()
        system_state = SYS_ROBOT_MODE
        PRINT "[FSM] 进入机器人模式，状态: SYS_READY -> SYS_ROBOT_MODE"

    ELSEIF cur_event = EVENT_JOINT THEN
        ' 单轴手动调整
        RUNTASK TASK_JOINT, MANNUAL_JOINT()
        motion_mode = MODE_JOINT_MANUAL
        active_task = TASK_JOINT
        system_state = SYS_RUNNING
        PRINT "[FSM] 单轴调整开始，状态: SYS_READY -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_CART_JOG THEN
        ' 拒绝：笛卡尔点动需要先进入机器人模式
        PRINT "[FSM] 拒绝点动: 请先进入机器人模式(EVENT_ROBOT_IN)"
        MODBUS_REG(REG_CMD_ERROR) = ERR_SYSTEM_NOT_READY

    ELSEIF cur_event = EVENT_TRAJ THEN
        ' 拒绝：轨迹执行需要先进入机器人模式
        PRINT "[FSM] 拒绝轨迹: 请先进入机器人模式(EVENT_ROBOT_IN)"
        MODBUS_REG(REG_CMD_ERROR) = ERR_SYSTEM_NOT_READY

    ELSEIF cur_event = EVENT_CTRL THEN
        ' 拒绝：闭环控制需要先进入机器人模式
        PRINT "[FSM] 拒绝闭环: 请先进入机器人模式(EVENT_ROBOT_IN)"
        MODBUS_REG(REG_CMD_ERROR) = ERR_SYSTEM_NOT_READY

    ELSEIF cur_event = EVENT_STOP THEN
        ' 当前无运动，无需停止
        PRINT "[FSM] 系统已空闲，无需停止"
        ' 状态不变

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        PRINT "[FSM] 急停触发，状态: SYS_READY -> SYS_ESTOP"

    ELSE
        ' 等待运动指令
        PRINT "[FSM] 系统就绪，等待运动指令。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_ROBOT_MODE — 机器人模式，可进行笛卡尔空间运动
' 允许: CART_JOG(点动), TRAJ(轨迹), JOINT(单轴, 完成后回SYS_SERVO_READY)
'       ROBOT_OUT(退出机器人模式), HOME(重新回零)
' ================================================================
SUB Handle_SYS_ROBOT_MODE(cur_event)
    IF cur_event = EVENT_CART_JOG THEN
        ' 笛卡尔空间点动
        RUNTASK TASK_CATR_JOG, CART_JOG()
        motion_mode = MODE_CART_JOG
        active_task = TASK_CATR_JOG
        system_state = SYS_RUNNING
        PRINT "[FSM] 笛卡尔点动开始，状态: SYS_ROBOT_MODE -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_TRAJ THEN
        ' 轨迹执行
        RUNTASK TASK_TRAJ, TRAJ_MOVE()
        motion_mode = MODE_TRAJECTORY
        active_task = TASK_TRAJ
        system_state = SYS_RUNNING
        PRINT "[FSM] 轨迹执行开始，状态: SYS_ROBOT_MODE -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_CTRL THEN
        ' 闭环控制（力控微调）：INT_CYCLE 周期任务，直接启动，无需 RUNTASK
        CTRL_MOVE()
        motion_mode = MODE_SENSOR_CLOSED
        active_task = TASK_CTRL_MOVE
        system_state = SYS_RUNNING
        PRINT "[FSM] 闭环控制开始，状态: SYS_ROBOT_MODE -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_JOINT THEN
        ' 单轴手动调整（完成后回 SYS_SERVO_READY，而非 SYS_ROBOT_MODE）
        RUNTASK TASK_JOINT, MANNUAL_JOINT()
        motion_mode = MODE_JOINT_MANUAL
        active_task = TASK_JOINT
        system_state = SYS_RUNNING
        PRINT "[FSM] 单轴调整开始，状态: SYS_ROBOT_MODE -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_ROBOT_OUT THEN
        ' 退出机器人模式，回到伺服就绪态
		RAPIDSTOP
        CANCEL  ' 用来退出机械手模式，CANCEL(0)才能完整退出机械手模式
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 退出机器人模式，状态: SYS_ROBOT_MODE -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_HOME THEN
        ' 重新执行回零
        RUNTASK TASK_HOEM, home_robot()
        motion_mode = MODE_HOME
        active_task = TASK_HOEM
        system_state = SYS_HOMING
        PRINT "[FSM] 重新回零，状态: SYS_ROBOT_MODE -> SYS_HOMING"

    ELSEIF cur_event = EVENT_STOP THEN
        ' 当前无运动，无需停止
        PRINT "[FSM] 系统已空闲，无需停止"

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        PRINT "[FSM] 急停触发，状态: SYS_ROBOT_MODE -> SYS_ESTOP"

    ELSE
        PRINT "[FSM] 机器人模式就绪，等待运动指令。当前事件:", cur_event
    ENDIF
END SUB


' ================================================================
' Handle_SYS_RUNNING — 正在执行运动任务
' 根据当前 motion_mode 判断是哪种运动类型，执行对应的停止/暂停/完成处理
' 允许: STOP, PAUSE, *_DONE(各类运动完成)
' ================================================================
SUB Handle_SYS_RUNNING(cur_event)
    IF cur_event = EVENT_STOP THEN
        ' 通用停止：根据当前运动模式停止对应任务
        IF motion_mode = MODE_JOINT_MANUAL THEN
            STOPTASK TASK_JOINT
            PRINT "[FSM] 单轴调整被停止"
        ELSEIF motion_mode = MODE_CART_JOG THEN
            STOPTASK TASK_CATR_JOG
            PRINT "[FSM] 笛卡尔点动被停止"
        ELSEIF motion_mode = MODE_TRAJECTORY THEN
            STOPTASK TASK_TRAJ
            PRINT "[FSM] 轨迹执行被停止"
        ELSEIF motion_mode = MODE_SENSOR_CLOSED THEN
            CTRL_MOVE_END()
            PRINT "[FSM] 闭环控制被停止"
        ENDIF

        RAPIDSTOP(4)  ' 取消当前运动和缓冲运动
        WAIT IDLE
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 运动停止，状态: SYS_RUNNING -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_JOINT_DONE THEN
        ' 单轴调整任务结束
        ' 注: 此事件由上位机停止指令触发，或任务内部检测到无新数据自动退出
        STOPTASK TASK_JOINT
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 单轴调整完成，状态: SYS_RUNNING -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_CART_JOG_DONE THEN
        ' 笛卡尔点动任务结束，回到机器人模式继续操作
        STOPTASK TASK_CATR_JOG
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_ROBOT_MODE
        PRINT "[FSM] 点动完成，状态: SYS_RUNNING -> SYS_ROBOT_MODE"

    ELSEIF cur_event = EVENT_TRAJ_DONE THEN
        ' 轨迹任务结束，回到机器人模式继续操作
        STOPTASK TASK_TRAJ
        ' 调用外部函数: traj_stop() — 确保运动完全停止
        ' CALL traj_stop()
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_ROBOT_MODE
        PRINT "[FSM] 轨迹执行完成，状态: SYS_RUNNING -> SYS_ROBOT_MODE"

    ELSEIF cur_event = EVENT_CTRL_DONE THEN
        ' 闭环控制结束（周期任务读到 cmd_id=0 后已自行停止，此处仅切换状态）
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_ROBOT_MODE
        PRINT "[FSM] 闭环控制完成，状态: SYS_RUNNING -> SYS_ROBOT_MODE"

    ELSEIF cur_event = EVENT_HOME_DONE THEN
        ' 回零任务在 RUNNING 期间完成（回零自身是运动）
        STOPTASK TASK_HOEM
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_READY
        PRINT "[FSM] 回零完成，状态: SYS_RUNNING -> SYS_READY"

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停：立即停止所有运动
        STOPTASK TASK_JOINT
        STOPTASK TASK_CATR_JOG
        STOPTASK TASK_TRAJ
        STOPTASK TASK_HOEM
        IF motion_mode = MODE_SENSOR_CLOSED THEN
            CTRL_MOVE_END()
        ENDIF
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        motion_mode = MODE_IDLE
        active_task = -1
        PRINT "[FSM] 运动中急停，状态: SYS_RUNNING -> SYS_ESTOP"

    ELSEIF cur_event = EVENT_PAUSE THEN
        ' 暂停当前运动任务
        PAUSETASK active_task
        PRINT "[FSM] 运动暂停，状态: SYS_RUNNING -> SYS_PAUSED"
        system_state = SYS_PAUSED
        ' motion_mode 保持不变，用于记录暂停前是什么运动类型

    ELSEIF cur_event = EVENT_JOINT THEN
        ' 已在运行单轴调整，拒绝重复
        PRINT "[FSM] 单轴调整任务已在进行中"
        ' 状态不变

    ELSEIF cur_event = EVENT_CART_JOG THEN
        ' 已在运行点动，拒绝重复
        PRINT "[FSM] 点动任务已在进行中"
        ' 状态不变

    ELSEIF cur_event = EVENT_TRAJ THEN
        ' 已在运行轨迹，拒绝重复
        PRINT "[FSM] 轨迹任务已在进行中"
        ' 状态不变

    ELSEIF cur_event = EVENT_CTRL THEN
        ' 已在运行闭环控制，拒绝重复
        PRINT "[FSM] 闭环控制任务已在进行中"
        ' 状态不变

    ELSEIF cur_event = EVENT_HOME THEN
        ' 已在运行其他任务，拒绝回零
        PRINT "[FSM] 请先停止当前运动再执行回零"
        ' 状态不变

    ELSE
        PRINT "[FSM] 运动执行中，等待完成或停止。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_PAUSED — 运动已暂停
' 允许: STOP(停止), RESUME(恢复)
' ================================================================
SUB Handle_SYS_PAUSED(cur_event)
    IF cur_event = EVENT_STOP THEN
        ' 停止当前暂停的任务
        IF motion_mode = MODE_TRAJECTORY THEN
            STOPTASK TASK_TRAJ
        ELSEIF motion_mode = MODE_JOINT_MANUAL THEN
            STOPTASK TASK_JOINT
        ELSEIF motion_mode = MODE_CART_JOG THEN
            STOPTASK TASK_CATR_JOG
        ELSEIF motion_mode = MODE_SENSOR_CLOSED THEN
            CTRL_MOVE_END()
        ENDIF

        RAPIDSTOP(1)  ' 取消缓冲运动
        WAIT IDLE
        motion_mode = MODE_IDLE
        active_task = -1
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 暂停任务被停止，状态: SYS_PAUSED -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_RESUME THEN
        ' 恢复当前暂停的任务
        RESUMETASK active_task
        system_state = SYS_RUNNING
        PRINT "[FSM] 运动恢复，状态: SYS_PAUSED -> SYS_RUNNING"

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 急停
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        motion_mode = MODE_IDLE
        active_task = -1
        PRINT "[FSM] 暂停中急停，状态: SYS_PAUSED -> SYS_ESTOP"

    ELSE
        PRINT "[FSM] 运动已暂停，等待恢复或停止。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_ESTOP — 急停状态
' 仅允许: ERROR_RESET(清除急停并恢复)
' ================================================================
SUB Handle_SYS_ESTOP(cur_event)
    IF cur_event = EVENT_ERROR_RESET THEN
        ' 清除急停，恢复轴使能
        ' 调用外部函数: reset_estop() — 重新使能所有轴
        ' CALL reset_estop()
        safety_state = 0
        MODBUS_REG(REG_SAFETY_STATE) = 0
        MODBUS_REG(REG_CMD_ERROR) = ERR_OK
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 急停已清除，状态: SYS_ESTOP -> SYS_SERVO_READY"
        PRINT "[FSM] 注意: 需要重新回零"

    ELSEIF cur_event = EVENT_STOP THEN
        ' 已在急停状态，无需额外操作
        PRINT "[FSM] 系统已处于急停状态"
        ' 状态不变

    ELSE
        ' 急停状态下，其他事件一律拒绝
        PRINT "[FSM] 急停状态下不可操作，请先清除急停。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' Handle_SYS_ERROR — 错误/报警状态
' 仅允许: ERROR_RESET(清除错误)
' ================================================================
SUB Handle_SYS_ERROR(cur_event)
    IF cur_event = EVENT_ERROR_RESET THEN
        ' 清除错误，恢复至伺服就绪态
        ' 调用外部函数: reset_error() — 清除驱动器错误，重新使能轴
        ' CALL reset_error()
        MODBUS_REG(REG_CMD_ERROR) = ERR_OK
        system_state = SYS_SERVO_READY
        PRINT "[FSM] 错误已清除，状态: SYS_ERROR -> SYS_SERVO_READY"

    ELSEIF cur_event = EVENT_ESTOP THEN
        ' 从错误状态进入急停
        ' 调用外部函数: enter_estop()
        ' CALL enter_estop()
        system_state = SYS_ESTOP
        safety_state = 2
        MODBUS_REG(REG_SAFETY_STATE) = 2
        PRINT "[FSM] 错误状态下急停，状态: SYS_ERROR -> SYS_ESTOP"

    ELSE
        ' 其他事件在错误状态下被过滤
        PRINT "[FSM] 系统存在错误，等待 RESET。当前事件:", cur_event
        ' 状态不变
    ENDIF
END SUB


' ================================================================
' 逻辑测试用 — 临时桩函数，完成后由真实模块替换
' 所有完成事件统一写入 REG_EVENT_L1
' ================================================================
SUB HOME_TASK()
    PRINT "Home Start."
    DELAY 5000
    PRINT "Home OVER"
    MODBUS_REG(REG_EVENT_L1) = EVENT_HOME_DONE

END SUB

SUB JOINT_TASK()
    PRINT "Joint Start."
    DELAY 5000
    PRINT "Joint OVER"
    MODBUS_REG(REG_EVENT_L1) = EVENT_JOINT_DONE

END SUB

SUB CART_JOG_TASK()
    PRINT "Cart Jog Start."
    DELAY 5000
    PRINT "Cart Jog OVER"
    MODBUS_REG(REG_EVENT_L1) = EVENT_CART_JOG_DONE

END SUB

SUB TRAJ_TASK()
    PRINT "Traj Start."
    DELAY 5000
    PRINT "Traj OVER"
    MODBUS_REG(REG_EVENT_L1) = EVENT_TRAJ_DONE

END SUB