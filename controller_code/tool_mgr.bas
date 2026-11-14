' ================================================================
' tool_mgr.bas - 末端工具独立控制
' 公转：EtherCAT轴5，ATYPE=66，使用DAC连续调速，DAC=0停止。
' 自转：步进电机硬件输出暂未配置，仅维护命令状态和回零互锁。
' ================================================================

GLOBAL SUB TOOL_HANDLE_EVENT(current_state, cur_event)
    LOCAL can_start

    ' 回零、错误和急停状态禁止启动；停止事件始终允许执行。
    can_start = 0
    IF current_state = SYS_SERVO_READY OR current_state = SYS_READY OR current_state = SYS_ROBOT_MODE OR current_state = SYS_RUNNING OR current_state = SYS_PAUSED THEN
        can_start = 1
    ENDIF

    IF cur_event = EVENT_REVOLUTION_START THEN
        IF can_start = 0 THEN
            MODBUS_REG(REG_CMD_ERROR) = ERR_SYSTEM_NOT_READY
            PRINT "[TOOL] 当前状态禁止启动公转电机"
            RETURN
        ENDIF
        TOOL_REVOLUTION_START()
    ELSEIF cur_event = EVENT_REVOLUTION_STOP THEN
        TOOL_REVOLUTION_STOP()
    ELSEIF cur_event = EVENT_ROTATION_START THEN
        IF can_start = 0 THEN
            MODBUS_REG(REG_CMD_ERROR) = ERR_SYSTEM_NOT_READY
            PRINT "[TOOL] 当前状态禁止启动自转电机"
            RETURN
        ENDIF
        TOOL_ROTATION_START()
    ELSEIF cur_event = EVENT_ROTATION_STOP THEN
        TOOL_ROTATION_STOP()
    ENDIF
END SUB


GLOBAL SUB TOOL_REVOLUTION_START()
    LOCAL target_rpm

    target_rpm = TABLE(TABLE_REVOLUTION_SPEED)
    IF target_rpm = 0 OR ABS(target_rpm) > MAX_TOOL_SPEED_RPM THEN
        MODBUS_REG(REG_CMD_ERROR) = ERR_TOOL_SPEED_INVALID
        PRINT "[TOOL] 公转转速非法，要求非零且绝对值不超过", MAX_TOOL_SPEED_RPM
        RETURN
    ENDIF

    revolution_speed_rpm = target_rpm
    BASE(TOOL_REVOLUTION_AXIS)
    ' 保留正负号以控制方向；EtherCAT速度模式的DAC单位为脉冲数/秒。
    DAC = revolution_speed_rpm * TOOL_DAC_PER_RPM
    BASE(0,1,2,3,4)  ' 恢复机器人轴组，避免影响主循环中的轴级指令
    revolution_running = 1
    MODBUS_REG(REG_CMD_ERROR) = ERR_OK
    PRINT "[TOOL] 公转启动/调速，rpm =", revolution_speed_rpm, "DAC =", revolution_speed_rpm * TOOL_DAC_PER_RPM
END SUB


GLOBAL SUB TOOL_REVOLUTION_STOP()
    BASE(TOOL_REVOLUTION_AXIS)
    DAC = 0
    BASE(0,1,2,3,4)  ' 工具停止不影响机器人轴组
    revolution_speed_rpm = 0
    revolution_running = 0
    PRINT "[TOOL] 公转停止"
END SUB


GLOBAL SUB TOOL_ROTATION_START()
    LOCAL target_rpm

    target_rpm = TABLE(TABLE_ROTATION_SPEED)
    IF target_rpm = 0 OR ABS(target_rpm) > MAX_TOOL_SPEED_RPM THEN
        MODBUS_REG(REG_CMD_ERROR) = ERR_TOOL_SPEED_INVALID
        PRINT "[TOOL] 自转转速非法，要求非零且绝对值不超过", MAX_TOOL_SPEED_RPM
        RETURN
    ENDIF

    rotation_speed_rpm = target_rpm
    ' TODO: 后续在此配置步进电机方向、脉冲频率并启动输出。
    rotation_running = 1
    MODBUS_REG(REG_CMD_ERROR) = ERR_OK
    PRINT "[TOOL] 自转启动命令已接收（硬件输出未配置），rpm =", rotation_speed_rpm
END SUB


GLOBAL SUB TOOL_ROTATION_STOP()
    ' TODO: 后续在此停止步进电机脉冲输出。
    rotation_speed_rpm = 0
    rotation_running = 0
    PRINT "[TOOL] 自转停止命令已接收（硬件输出未配置）"
END SUB


GLOBAL SUB TOOL_CAN_HOME()
    IF revolution_running <> 0 OR rotation_running <> 0 THEN
        RETURN 0
    ENDIF
    RETURN 1
END SUB


GLOBAL SUB TOOL_ALL_STOP()
    TOOL_REVOLUTION_STOP()
    TOOL_ROTATION_STOP()
END SUB
