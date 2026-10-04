' 独立开一个任务运行？？
WHILE 1
    pc_heartbeat = MODBUS_REG(REG_PC_HEARTBEAT)

    ' 例：检测 Qt 心跳是否变化
    IF pc_heartbeat = last_pc_heartbeat THEN
        heartbeat_count = heartbeat_count + 1
    ELSE
        heartbeat_count = 0
        last_pc_heartbeat = pc_heartbeat
    ENDIF

    IF heartbeat_count > 200 THEN
        safety_state = 2  ' communication lost
        MODBUS_REG(REG_SAFETY_STATE) = safety_state
        ' 暂时不立刻 RAPIDSTOP，后续根据运行状态处理
    ENDIF

    DELAY(10)
WEND


' AXISSTATUS(axis)
' AXIS_STOPREASON(axis)
' IN(急停输入)
' IN(限位输入)
' DRIVE_STATUS(axis)
' FE(axis)