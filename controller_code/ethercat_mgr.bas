' 轴映射关系 实轴0-1-2-3-4
GLOBAL SUB Ecat_Init()
    LOCAL node_num, temp_axis, node_axis_num, drive_vender, drive_device, drive_alias
    temp_axis = 0
    LOCAL i_axis

    RAPIDSTOP(2)
    WAIT IDLE

    bus_initstate = -1
    bus_total_axis_num = 0
    SLOT_STOP(BUS_SLOT)
    DELAY(200)

    ' -------------总线扫描-------------
    SLOT_SCAN(BUS_SLOT)
    IF RETURN THEN
        PRINT "总线扫描成功","连接设备数："NODE_COUNT(BUS_SLOT)
        PRINT "期望连接设备数："BUS_NODE_NUM

        IF NODE_COUNT(BUS_SLOT) <> BUS_NODE_NUM THEN
            bus_initstate = 0
            RETURN
        ENDIF

        ' -------------EL3702采集资源映射-------------
        ' 节点6为EK1100，节点7为EL3702；两者轴数均为0，不参与下方6个电机轴映射。
        force_use_ain = 0
        IF NODE_INFO(BUS_SLOT, FORCE_NODE, 12) >= 2 THEN
            NODE_AIO(BUS_SLOT, FORCE_NODE, 3) = FORCE_AIN_BASE
            IF NODE_AIO(BUS_SLOT, FORCE_NODE, 3) = FORCE_AIN_BASE THEN
                force_use_ain = 1
                PRINT "EL3702使用标准AIN，起始通道："FORCE_AIN_BASE
            ENDIF
        ENDIF

        ' 无标准AIN时仅接受默认n=1的8字节PDO，防止其他布局造成偏移误读。
        IF force_use_ain = 0 THEN
            IF NODE_INFO(BUS_SLOT, FORCE_NODE, 14) <> FORCE_PDO_BYTES THEN
                bus_initstate = 0
                PRINT "EL3702输入PDO不是8字节，请检查超采样和PDO配置"
                RETURN
            ENDIF
            PRINT "EL3702使用PDO原始字节，TABLE起始："FORCE_PDO_TABLE_BEGIN
        ENDIF

        ' -------------映射轴号-------------
        ' 当前的情况是每个驱动器是一个设备，每个驱动器下有一个轴
        FOR node_num = 0 TO NODE_COUNT(BUS_SLOT) - 1
            node_axis_num = NODE_AXIS_COUNT(BUS_SLOT, node_num)
            IF node_axis_num <> 0 THEN
                FOR i_axis = 0 TO node_axis_num - 1 STEP 1
                    AXIS_ADDRESS(bus_total_axis_num) = bus_total_axis_num + 1  ' 加一是格式规定
                    IF bus_total_axis_num = TOOL_REVOLUTION_AXIS THEN
                        ATYPE(bus_total_axis_num) = 66  ' 公转电机：EtherCAT速度模式
                        DRIVE_PROFILE(bus_total_axis_num) = 20  ' 速度模式PDO
                    ELSE
                        ATYPE(bus_total_axis_num) = 65  ' 机器人轴：位置模式
                        DRIVE_PROFILE(bus_total_axis_num) = 1  ' 位置模式+扭矩反馈
                    ENDIF
                    bus_total_axis_num = bus_total_axis_num + 1
                NEXT
            ENDIF
        NEXT

        IF bus_total_axis_num <> 6 THEN
            bus_initstate = 0
            PRINT "总线轴数异常，期望6轴，实际："bus_total_axis_num
            RETURN
        ENDIF

        DISABLE_GROUP(0,1,2,3,4,5)  ' 共同关闭使能，包含公转轴

        PRINT "轴号映射完成","连接总轴数："bus_total_axis_num
        DELAY 200

        ' -------------启动总线-------------
        SLOT_START(BUS_SLOT)
        IF RETURN THEN
            WDOG = 1  ' 使能所有轴（既需要单轴使能，也需要所有轴使能）

            BASE(TOOL_REVOLUTION_AXIS)
            DAC = 0  ' 公转轴上电默认停止

            ' -------------清除驱动器错误-------------
            FOR i = 0 TO bus_total_axis_num - 1
                BASE(i)
                DRIVE_CLEAR(0)
                DELAY 50

                DATUM(0)
                DELAY 100
                AXIS_ENABLE = 1  ' 单轴使能
            NEXT

            bus_initstate = 1
        
        ELSE
            bus_initstate = 0
            PRINT "总线开启失败"
        ENDIF

    ELSE
        bus_initstate = 0
        PRINT "总线扫描失败"

    ENDIF
END SUB