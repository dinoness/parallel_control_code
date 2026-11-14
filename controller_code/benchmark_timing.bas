' 临时基准测试：不执行运动，不修改控制参数
' TABLE 900-915 为预留测试地址
' 所有测试循环均包含与空循环相同的两次赋值，使用 t_empty 扣除公共开销

GLOBAL SUB BENCHMARK()
    DIM bench_array(16)
    LOCAL i, n
    LOCAL t_empty, t_test, t_table5
    LOCAL bench_value, bench_addr

    n = 10000

    PRINT "=================================="
    PRINT "开始耗时基准测试，循环次数: ", n
    PRINT "单次测试与批量测试分开统计，批量测试同时输出每通道耗时"

    ' 1. 公共空循环基准
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
    NEXT
    t_empty = TIME_TICKUS
    PRINT "空循环总耗时(us): ", t_empty

    ' 2. TABLE 单地址写入
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900 + bench_addr) = bench_value
    NEXT
    t_test = TIME_TICKUS
    PRINT "TABLE 单地址写入每次(us): ", (t_test - t_empty) / n

    ' 3. 普通数组单元素写入
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_array(bench_addr) = bench_value
    NEXT
    t_test = TIME_TICKUS
    PRINT "数组单元素写入每次(us): ", (t_test - t_empty) / n

    ' 4. TABLE 16 通道批量写入：每批共 16 次通道写操作
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
    NEXT
    t_test = TIME_TICKUS
    PRINT "TABLE 16通道批量写每批(us): ", (t_test - t_empty) / n
    PRINT "TABLE 16通道批量写每通道(us): ", (t_test - t_empty) / n / 16

    ' 5. 普通数组 16 通道批量写入
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_array(0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
    NEXT
    t_test = TIME_TICKUS
    PRINT "数组16通道批量写每批(us): ", (t_test - t_empty) / n
    PRINT "数组16通道批量写每通道(us): ", (t_test - t_empty) / n / 16

    ' 6. TABLE 5 通道批量写入，作为 5 轴读取测试的公共参照
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,0,1,2,3,4)
    NEXT
    t_test = TIME_TICKUS
    t_table5 = t_test
    PRINT "TABLE 5通道批量写每批(us): ", (t_test - t_empty) / n
    PRINT "TABLE 5通道批量写每通道(us): ", (t_test - t_empty) / n / 5

    ' 7. 5 个 MPOS 读取并批量写入 TABLE
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,MPOS(0),MPOS(1),MPOS(2),MPOS(3),MPOS(4))
    NEXT
    t_test = TIME_TICKUS
    PRINT "MPOS+TABLE 5通道每批(us): ", (t_test - t_empty) / n
    PRINT "MPOS+TABLE 5通道每通道(us): ", (t_test - t_empty) / n / 5
    PRINT "MPOS读取附加耗时每通道(us): ", (t_test - t_table5) / n / 5

    ' 8. 5 个 ENCODER 读取并批量写入 TABLE
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,ENCODER(0),ENCODER(1),ENCODER(2),ENCODER(3),ENCODER(4))
    NEXT
    t_test = TIME_TICKUS
    PRINT "ENCODER+TABLE 5通道每批(us): ", (t_test - t_empty) / n
    PRINT "ENCODER+TABLE 5通道每通道(us): ", (t_test - t_empty) / n / 5
    PRINT "ENCODER读取附加耗时每通道(us): ", (t_test - t_table5) / n / 5

    ' 9. 5 个 DRIVE_TORQUE 读取并批量写入 TABLE
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,DRIVE_TORQUE(0),DRIVE_TORQUE(1),DRIVE_TORQUE(2),DRIVE_TORQUE(3),DRIVE_TORQUE(4))
    NEXT
    t_test = TIME_TICKUS
    PRINT "DRIVE_TORQUE+TABLE 5通道每批(us): ", (t_test - t_empty) / n
    PRINT "DRIVE_TORQUE+TABLE 5通道每通道(us): ", (t_test - t_empty) / n / 5
    PRINT "DRIVE_TORQUE读取附加耗时每通道(us): ", (t_test - t_table5) / n / 5

    ' 10. 5 个 DPOS 读取并批量写入 TABLE，仅在机械手连接有效时执行
    IF ee_valid = 1 THEN
        TIME_TICKUS = 0
        FOR i = 0 TO n - 1
            bench_value = i
            bench_addr = i MOD 16
            TABLE(900,DPOS(6),DPOS(7),DPOS(8),DPOS(9),DPOS(10))
        NEXT
        t_test = TIME_TICKUS
        PRINT "DPOS+TABLE 5通道每批(us): ", (t_test - t_empty) / n
        PRINT "DPOS+TABLE 5通道每通道(us): ", (t_test - t_empty) / n / 5
        PRINT "DPOS读取附加耗时每通道(us): ", (t_test - t_table5) / n / 5
    ELSE
        PRINT "ee_valid=0，跳过 DPOS 测试"
    ENDIF

    ' 清理测试地址，不影响其他控制参数
    TABLE(900,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)

    PRINT "基准测试完成"
    PRINT "=================================="
END SUB
