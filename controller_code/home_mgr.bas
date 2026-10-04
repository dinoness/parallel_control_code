' 需要测试多轴回零的逻辑
' DATUM指令说明
' 21表示参考电机驱动器的回零模式配置，后面接具体的模式代号
' 模式17：电机负向驱动，以反向限位开关作为停止点
' 模式28：电机负向驱动，以零位开关作为停止点，停在原点开关的正侧
' 模式29：电机负向驱动，以零位开关作为停止点，停在原点开关的负侧

' DRIVE_STATUS
' 在EtherCAT中，用于获取数据字典6041下的值，每个位(bit)代表的信息参考驱动器手册

' READ_BIT2(位数， 字典索引)


GLOBAL SUB HOME_ROBOT()
    IF bus_initstate <> 1 THEN
        PRINT "总线未初始化"
        END SUB
    ENDIF

    IF bus_total_axis_num <> 5 THEN
        PRINT "轴数不为5"
        END SUB
    ENDIF

    dim i_axis
    FOR i_axis = 0 TO 2 STEP 1
        BASE(i_axis)
        creep = 1 * LENGTH_UNIT  ' 回零反找时的速度
        speed = 3 * LENGTH_UNIT
        accel = 10 * LENGTH_UNIT
        decel = 10 * LENGTH_UNIT
        DATUM(21, 29)
    NEXT

	FOR i_axis = 3 TO 4 STEP 1
		BASE(i_axis)
		creep = 1 * LENGTH_UNIT  ' 回零反找时的速度
		speed = 3 * LENGTH_UNIT
		accel = 10 * LENGTH_UNIT
		decel = 10 * LENGTH_UNIT
		DATUM(21, 29)
	NEXT

    
    ' 等待回零完成
	FOR i_axis = 0 TO (bus_total_axis_num - 1) STEP 1
		WAIT IDLE(i_axis)
	NEXT
	
    ' 等待驱动器状态更新
	DELAY(1000)

	' 检查是否回零成功
    LOCAL num_home_axis
	num_home_axis = 0
    FOR i_axis = 0 TO (bus_total_axis_num - 1) STEP 1
        TABLE(TABLE_DRIVER_STATUE) = DRIVE_STATUS(i_axis)
        IF READ_BIT2(15, TABLE(TABLE_DRIVER_STATUE)) THEN
            num_home_axis = num_home_axis + 1
        ENDIF
		' PRINT "AXIS ID:"(i_axis+1)
		' PRINT "Home State:"READ_BIT2(15, TABLE(TABLE_DRIVER_STATUE))
    NEXT

    IF num_home_axis <> bus_total_axis_num THEN
        MODBUS_REG(REG_EVENT_L1) = EVENT_HOME_FAILED
    ELSE
        MODBUS_REG(REG_EVENT_L1) = EVENT_HOME_DONE
    ENDIF
END SUB