' ================================================================
' status_mgr.bas - 状态采样上传（INT_CYCLE 周期中断任务，常驻）
' 功能：每 STATUS_SAMPLE_DIV(5) 个伺服周期采样一帧（1kHz 总线 → 200Hz），
'       写入 TABLE 环形缓冲，供上位机每 0.1s 批量读取（每窗口约 20 帧）。
' 数据区：TABLE_STATUS_BASE(21000) 起
'   header（2 个 float）：[0]=status_frame_counter, [1]=status_write_index
'   环形帧区：STATUS_RING_FRAMES(512) 帧 × STATUS_CHANNELS(24) 通道
'   每帧布局：
'     [0-4]   支链伸缩量 dL(um)，MPOS 相对 dl_base 基准
'     [5-9]   电机编码器原始脉冲 ENCODER
'     [10-14] 电机扭矩 DRIVE_TORQUE（0x6077，千分比 ‰）
'     [15-19] 末端位姿 x,y,z,phi,theta（DPOS(6..10)，仅机器人模式有效）
'     [20]    ee_valid 标志（1=末端位姿有效，0=无效）
'     [21-23] 预留
' 同步约定：先写完一帧数据，最后更新 header，保证上位机不会读到半成品帧。
' 运行方式：STATUS_INIT() 注册 INT_CYCLE 中断（任务号 TASK_STATUS），
'           每个 SERVO_PERIOD 调用一次 STATUS_SAMPLE，常驻不退出。
' 注意：周期执行的 SUB 必须精简，本实现每周期仅做数组读写，无阻塞。
' ================================================================

' 本模块使用的全局变量（dl_base、ee_valid、status_frame_counter、status_write_index）
' 声明在 global_config.bas 的 GLOBAL_DEF() 中。
' 注意：RTBasic 中非 AutoRun 文件的文件级 GLOBAL 声明不会被执行，
' 全局变量必须经 main.bas 调用的 GLOBAL_DEF() 注册，否则运行时报 Array index over max。

' 初始化：标定 dL 基准、清零状态与 header、注册周期中断
GLOBAL SUB STATUS_INIT()
    PRINT "状态采样任务启动"
    DL_BASE_CAPTURE()
    ee_valid = 0
    status_frame_counter = 0
    status_write_index = 0
    status_tick = 0
    ' header 清零
    TABLE(TABLE_STATUS_BASE) = 0
    TABLE(TABLE_STATUS_BASE + 1) = 0
    ' 注册伺服周期中断（INT_ENABLE=1 已在 main.bas 全局打开，无需重复设置）
    INT_CYCLE(1, TASK_STATUS, STATUS_SAMPLE)
END SUB

' 标定 dL 零位基准：回零完成后调用，使 dL 以回零点为零位
GLOBAL SUB DL_BASE_CAPTURE()
    LOCAL i
    FOR i = 0 TO 4
        dl_base(i) = MPOS(i)
    NEXT
END SUB

' 周期采样入口：每个 SERVO_PERIOD 执行一次，必须精简
GLOBAL SUB STATUS_SAMPLE()
    LOCAL i, base_addr

    ' 分频：每 STATUS_SAMPLE_DIV 个伺服周期才写一帧，其余周期直接返回
    status_tick = status_tick + 1
    IF status_tick >= STATUS_SAMPLE_DIV THEN
        status_tick = 0
        base_addr = TABLE_STATUS_BASE + STATUS_HEADER_FLOATS + status_write_index * STATUS_CHANNELS

        FOR i = 0 TO 4
            TABLE(base_addr + i) = MPOS(i) - dl_base(i)          ' [0-4] 支链伸缩量 dL(um)
            ' 若实测 ATYPE=65 不支持 ENCODER，改用 MPOS(i)*UNITS(i)
            TABLE(base_addr + 5 + i) = ENCODER(i)                ' [5-9] 编码器原始脉冲
            TABLE(base_addr + 10 + i) = DRIVE_TORQUE(i)          ' [10-14] 电机扭矩（‰）
        NEXT

        ' [15-20] 末端位姿：仅机器人模式（CONNFRAME 已建立）下有效，
        ' ee_valid=0 时虚轴 6-10 未配置，不能读 DPOS(6..10)
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
