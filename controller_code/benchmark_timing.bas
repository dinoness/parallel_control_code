' 临时基准测试：不执行运动，不修改控制参数
' TABLE 900-959 为预留测试地址（437-999 预留区内）
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

' ================================================================
' 优化方向基准测试：RUNTASK 10, BENCHMARK_OPT()
' 对应四项优化疑问：
'   问题1：力控参数逐条 TABLE 读 vs 批量缓存 vs 编译期常量
'   问题2：状态帧一句话直写 vs 数组填充+DMCPY 两句拷贝
'   问题3：GLOBAL SUB 调用本身的开销（评估函数合并收益）
'   问题4：GLOBAL/LOCAL 变量读与 IF 判定开销（评估冗余判定清理收益）
' 注意：DMCPY 段若报语法/指令错误，注释对应循环即可，不影响其余测试
' ================================================================

' 空调用基准用空过程
GLOBAL SUB BENCH_EMPTY_SUB()
END SUB

GLOBAL SUB BENCHMARK_OPT()
    DIM bench_a(16), bench_b(16)
    GLOBAL bench_g
    GLOBAL CONST BENCH_K = 50.0
    LOCAL i, n, bench_local
    LOCAL t_empty, t_test
    LOCAL bench_value, bench_addr

    n = 10000
    bench_g = 1
    bench_local = 1

    PRINT "=================================="
    PRINT "优化方向基准测试，循环次数: ", n

    ' 0. 公共空循环基准
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
    NEXT
    t_empty = TIME_TICKUS
    PRINT "空循环总耗时(us): ", t_empty

    ' ---------- 问题1：力控参数读取方式 ----------
    ' 1.1 TABLE 单地址读取（现状 FORCE_SAMPLE 逐条读的方式）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = TABLE(900 + bench_addr)
    NEXT
    t_test = TIME_TICKUS
    PRINT "TABLE 单地址读取每次(us): ", (t_test - t_empty) / n

    ' 1.2 一条批量数组赋值读入 16 个 TABLE（一句话缓存方案）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_a(0, TABLE(900), TABLE(901), TABLE(902), TABLE(903), TABLE(904), TABLE(905), TABLE(906), TABLE(907), TABLE(908), TABLE(909), TABLE(910), TABLE(911), TABLE(912), TABLE(913), TABLE(914), TABLE(915))
    NEXT
    t_test = TIME_TICKUS
    PRINT "批量数组赋值读TABLE 16通道每批(us): ", (t_test - t_empty) / n
    PRINT "批量数组赋值读TABLE 每通道(us): ", (t_test - t_empty) / n / 16

    ' 1.3 DMCPY TABLE→数组 16 通道
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        DMCPY bench_a(0), TABLE(900), 16
    NEXT
    t_test = TIME_TICKUS
    PRINT "DMCPY TABLE→数组 16通道每批(us): ", (t_test - t_empty) / n
    PRINT "DMCPY TABLE→数组 每通道(us): ", (t_test - t_empty) / n / 16

    ' 1.4 数组元素读取（参数缓存进数组后每次使用的成本）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = bench_a(bench_addr)
    NEXT
    t_test = TIME_TICKUS
    PRINT "数组元素读取每次(us): ", (t_test - t_empty) / n

    ' 1.5 编译期常量参与运算（理论下限，运行时不可调）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = i * BENCH_K
    NEXT
    t_test = TIME_TICKUS
    PRINT "常量参与运算每次(us): ", (t_test - t_empty) / n

    ' ---------- 问题2：状态帧写入方式 ----------
    ' 2.1 现状：一句话 16 参数直写（MPOS/DRIVE_TORQUE/数组内联）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900, MPOS(0), MPOS(1), MPOS(2), MPOS(3), MPOS(4), DRIVE_TORQUE(0), DRIVE_TORQUE(1), DRIVE_TORQUE(2), DRIVE_TORQUE(3), DRIVE_TORQUE(4), bench_a(0), bench_a(1), bench_a(2), bench_a(3), bench_a(4), bench_a(5))
    NEXT
    t_test = TIME_TICKUS
    PRINT "现状16参数一句话直写每批(us): ", (t_test - t_empty) / n

    ' 2.2 两句方案：先批量填充数组，再 DMCPY 到 TABLE
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_b(0, MPOS(0), MPOS(1), MPOS(2), MPOS(3), MPOS(4), DRIVE_TORQUE(0), DRIVE_TORQUE(1), DRIVE_TORQUE(2), DRIVE_TORQUE(3), DRIVE_TORQUE(4), bench_a(0), bench_a(1), bench_a(2), bench_a(3), bench_a(4), bench_a(5))
        DMCPY TABLE(900), bench_b(0), 16
    NEXT
    t_test = TIME_TICKUS
    PRINT "数组填充+DMCPY 两句合计每批(us): ", (t_test - t_empty) / n

    ' 2.3 DMCPY 纯拷贝成本：数组→TABLE 16 通道
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        DMCPY TABLE(900), bench_a(0), 16
    NEXT
    t_test = TIME_TICKUS
    PRINT "DMCPY 数组→TABLE 16通道每批(us): ", (t_test - t_empty) / n
    PRINT "DMCPY 数组→TABLE 每通道(us): ", (t_test - t_empty) / n / 16

    ' 2.4 TABLE 16 通道纯数值批量写（与旧基准对照，供跨次比较）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        TABLE(900,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
    NEXT
    t_test = TIME_TICKUS
    PRINT "TABLE 16通道纯数值批量写每批(us): ", (t_test - t_empty) / n

    ' ---------- 问题3：GLOBAL SUB 空调用开销 ----------
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        BENCH_EMPTY_SUB()
    NEXT
    t_test = TIME_TICKUS
    PRINT "空SUB调用每次(us): ", (t_test - t_empty) / n

    ' ---------- 问题4：变量读与判定开销 ----------
    ' 4.1 GLOBAL 变量读
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = bench_g
    NEXT
    t_test = TIME_TICKUS
    PRINT "GLOBAL 变量读取每次(us): ", (t_test - t_empty) / n

    ' 4.2 LOCAL 变量读
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = bench_local
    NEXT
    t_test = TIME_TICKUS
    PRINT "LOCAL 变量读取每次(us): ", (t_test - t_empty) / n

    ' 4.3 单条件 IF 判定（条件恒真，含一句赋值）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        IF bench_g = 1 THEN
            bench_value = 1
        ENDIF
    NEXT
    t_test = TIME_TICKUS
    PRINT "单条件IF(含赋值)每次(us): ", (t_test - t_empty) / n

    ' 4.4 双条件 IF AND 判定（条件恒真，含一句赋值）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        IF bench_g = 1 AND bench_local = 1 THEN
            bench_value = 1
        ENDIF
    NEXT
    t_test = TIME_TICKUS
    PRINT "双条件IF-AND(含赋值)每次(us): ", (t_test - t_empty) / n

    ' 清理测试地址，不影响其他控制参数
    TABLE(900,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)

    PRINT "优化方向基准测试完成"
    PRINT "=================================="
END SUB

' ================================================================
' 力控热路径专项基准：RUNTASK 10, BENCHMARK_FORCE()
' 比较FOR循环逐条TABLE读与一条批量数组赋值；阻抗算法已改为C内部函数，
' 不再通过独立CFUNC暴露，因此本测试不直接调用阻抗入口。
' ================================================================
GLOBAL SUB BENCHMARK_FORCE()
    DIM bench_b(8)
    LOCAL i, n, bench_j
    LOCAL t_empty, t_test
    LOCAL bench_value, bench_addr

    n = 10000

    PRINT "=================================="
    PRINT "力控热路径基准测试，循环次数: ", n

    ' 0. 公共空循环基准
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
    NEXT
    t_empty = TIME_TICKUS
    PRINT "空循环总耗时(us): ", t_empty

    ' 1.1 旧方式：FOR 循环 5 次逐条 TABLE 读（修改前的 PTABS 消费）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        FOR bench_j = 0 TO 4
            bench_b(bench_j) = TABLE(900 + bench_j)
        NEXT
    NEXT
    t_test = TIME_TICKUS
    PRINT "FOR循环5次TABLE读每次(us): ", (t_test - t_empty) / n

    ' 1.2 新方式：一条批量数组赋值读 5 个 TABLE
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_b(0, TABLE(900), TABLE(901), TABLE(902), TABLE(903), TABLE(904))
    NEXT
    t_test = TIME_TICKUS
    PRINT "批量赋值5次TABLE读每次(us): ", (t_test - t_empty) / n

    PRINT "力控热路径基准测试完成"
    PRINT "=================================="
END SUB

' ================================================================
' 运动指令下发基准：RUNTASK 10, BENCHMARK_MOVE()
' 前置条件：已完成回零并进入机器人模式（ee_valid=1），系统空闲无运动
' 测试对象：BASE 切换、MODBUS_REG 读、REMAIN_BUFFER 判断、
'          MOVE_PTABS(ticks=1) 含 CFRAME 逆解的下发开销
' 注意：MOVE_PTABS 测试向虚拟轴 6-10 缓冲写入 200 条零位移保持点，
'       目标=当前 DPOS，不产生实际位移；结束后 RAPIDSTOP(2) 清空缓冲
' ================================================================
GLOBAL SUB BENCHMARK_MOVE()
    LOCAL i, n, t_empty, t_test
    LOCAL bench_value, bench_addr

    n = 10000

    PRINT "=================================="
    PRINT "运动指令下发基准测试，循环次数: ", n

    ' 0. 公共空循环基准
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
    NEXT
    t_empty = TIME_TICKUS
    PRINT "空循环总耗时(us): ", t_empty

    ' 1. BASE(6,7,8,9,10) 轴组切换
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        BASE(6,7,8,9,10)
    NEXT
    t_test = TIME_TICKUS
    PRINT "BASE(6-10) 每次(us): ", (t_test - t_empty) / n

    ' 2. MODBUS_REG 单地址读取（组状态检查的语句开销）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        bench_value = MODBUS_REG(50)
    NEXT
    t_test = TIME_TICKUS
    PRINT "MODBUS_REG 读取每次(us): ", (t_test - t_empty) / n

    ' 3. REMAIN_BUFFER 缓冲余量判断（守卫语句的开销）
    TIME_TICKUS = 0
    FOR i = 0 TO n - 1
        bench_value = i
        bench_addr = i MOD 16
        IF REMAIN_BUFFER AXIS(6) <= 2 THEN
            bench_value = 1
        ENDIF
    NEXT
    t_test = TIME_TICKUS
    PRINT "REMAIN_BUFFER 判断每次(us): ", (t_test - t_empty) / n

    ' 4. MOVE_PTABS(ticks=1) 下发：含 CFRAME 逆解与缓冲写入，限发 200 条
    '    空循环基准按 200 次折算：t_empty / 50
    IF ee_valid = 1 THEN
        BASE(6,7,8,9,10)
        TIME_TICKUS = 0
        FOR i = 0 TO 199
            bench_value = i
            bench_addr = i MOD 16
            MOVE_PTABS(1, DPOS(6), DPOS(7), DPOS(8), DPOS(9), DPOS(10))
        NEXT
        t_test = TIME_TICKUS
        RAPIDSTOP(2)
        DELAY(200)
        PRINT "MOVE_PTABS(ticks=1) 下发每次(us): ", (t_test - t_empty / 50) / 200
    ELSE
        PRINT "ee_valid=0，跳过 MOVE_PTABS 测试（请先回零并进入机器人模式）"
    ENDIF

    PRINT "运动指令下发基准测试完成"
    PRINT "=================================="
END SUB
