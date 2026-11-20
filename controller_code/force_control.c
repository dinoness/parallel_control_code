#include "force_control.h"

static TYPE_TABLE *g_force_table;
static ForceMgrParams g_force_params;
static ImpedanceParams g_impedance;
static ForceCtrlState g_force_state;
static fp32 g_tool;  // 水平标定原始码换算后的力，单位N
static uint8 g_initialized;

// 力采集状态：采集、1kHz上传、力控计算三者同周期同源，不经过BASIC中转
static fp32 g_fz_raw;               // 本周期 Fz 有符号原始码
static uint32 g_stream_count;       // 高频环形区已提交帧数
static uint32 g_stream_write_index; // 高频环形区下一帧写入位置

// 清零写回BASIC的五维调整量
static void ClearAdjustmentOutput(void)
{
    int i;

    if(g_force_table == 0)
    {
        return;
    }

    for(i = 0; i < 5; ++i)
    {
        g_force_table[FORCE_TABLE_ADJ_BEGIN + i] = 0.0;
    }
}

// 阻抗控制，输入和参考力单位为N，输出调整量单位为um
static fp32 ImpedanceCtrl(fp32 f_sensor, fp32 f_ref, fp32 v_max)
{
    fp32 f_err = f_sensor - f_ref;
    fp32 e0;
    fp32 delta_e;

    e0 = g_impedance.c_f * f_err
         + g_impedance.c_e1 * g_impedance.e1
         - g_impedance.c_e2 * g_impedance.e2;

    // 单周期位移限幅
    delta_e = e0 - g_impedance.e1;
    if(fabs(delta_e) > v_max)
    {
        e0 = g_impedance.e1 + ((delta_e > 0.0f) ? v_max : -v_max);
    }

    // 总位移限幅后再更新历史，避免历史值与实际输出不一致
    if(fabs(e0) > g_force_params.adj_max)
    {
        e0 = (e0 > 0.0f) ? g_force_params.adj_max : -g_force_params.adj_max;
    }

    g_impedance.e2 = g_impedance.e1;
    g_impedance.e1 = e0;

    return e0 * 1e6f;
}

int CtrlInit(void)
{
    fp32 den;

    g_force_table = motionrt_gettableptr();
    if(g_force_table == 0)
    {
        g_initialized = 0;
        return FORCE_CTRL_ERROR;
    }

    g_tool = 0.0f;

    // 高频环形区由 C 独占写入，初始化时清零状态并提交空 header
    g_fz_raw = 0.0f;
    g_stream_count = 0;
    g_stream_write_index = 0;
    g_force_table[FORCE_STREAM_TABLE_BASE] = 0.0;
    g_force_table[FORCE_STREAM_TABLE_BASE + 1] = 0.0;

    // 固定阻抗参数
    g_impedance.m = 2.5f;
    g_impedance.b = 80.0f;
    g_impedance.k = 1000.0f;
    g_impedance.T = 0.001f;

    // 缓存离散公式系数，周期计算中不再重复求幂和除法
    den = g_impedance.m + g_impedance.T * g_impedance.b
          + g_impedance.T * g_impedance.T * g_impedance.k;
    g_impedance.c_f = g_impedance.T * g_impedance.T / den;
    g_impedance.c_e1 = (g_impedance.T * g_impedance.b + 2.0f * g_impedance.m) / den;
    g_impedance.c_e2 = g_impedance.m / den;

    g_initialized = 1;
    CtrlClear();
    return FORCE_CTRL_OK;
}

int CtrlPara(fp32 f_target, fp32 contact_threshold, fp32 ramp_step, fp32 max_adjust)
{
    if(g_initialized == 0 || g_force_table == 0)
    {
        return FORCE_CTRL_ERROR;
    }

    if(f_target < 0.0f || contact_threshold <= 0.0f
       || ramp_step <= 0.0f || max_adjust <= 0.0f)
    {
        return FORCE_CTRL_PARAM_ERROR;
    }

    // 新力控段不继承上一段的接触状态和阻抗历史
    CtrlClear();

    g_force_params.f_target = f_target;
    g_force_params.contact_threshold = contact_threshold;
    g_force_params.ramp_step = ramp_step;
    g_force_params.adj_max = max_adjust * 1e-6f;

    // 水平标定值与传感器输入均为原始码，这里统一换算为N
    g_tool = (fp32)g_force_table[FORCE_TABLE_G_TOOL] * FORCE_SENSOR_SCALE;

    return FORCE_CTRL_OK;
}

int CtrlClear(void)
{
    g_force_params.f_target = 0.0f;
    g_force_params.contact_threshold = 0.0f;
    g_force_params.ramp_step = 0.0f;
    g_force_params.adj_max = 0.0f;

    g_force_state.f_ref_effective = 0.0f;
    g_force_state.contact_count = 0;
    g_force_state.flag_contacted = 0;

    g_impedance.e1 = 0.0f;
    g_impedance.e2 = 0.0f;

    ClearAdjustmentOutput();
    return FORCE_CTRL_OK;
}


// 每伺服周期由 BASIC 调用一次：使能判断、AIN 采集、无分支补码转换、1kHz 环形区写入。
// 返回采样计数（BASIC 的 10ms 低频维护据此做断流检测）；模块未初始化返回 -1。
int ForceSample(void)
{
    int32 raw;

    if(g_initialized == 0 || g_force_table == 0)
    {
        return -1;
    }

    // 使能判断在 C 内完成：TABLE(410)=0 时不采样、计数停，保持断流检测语义。
    // 节点掉线由 BASIC 低频维护的 NODE_STATUS 置 INVALID 拦截，不在此处判断。
    if(g_force_table[FORCE_TABLE_CFG_BEGIN + FORCE_CFG_ENABLE_OFFSET] == 0.0)
    {
        return (int)g_stream_count;
    }

    // 与 BASIC 旧逻辑相同的算术回绕：ZMC 按有符号或无符号返回 16 位原始码都正确
    raw = (int32)motionrt_getad(FORCE_AIN_NUM);
    raw = ((raw + 32768) & 0xFFFF) - 32768;
    g_fz_raw = (fp32)raw;

    // 1kHz 环形区：先写数据、后提交 header，协议不变，Qt 读取链路无需改动
    g_force_table[FORCE_STREAM_DATA_BEGIN + g_stream_write_index] = (TYPE_TABLE)raw;
    g_stream_write_index = (g_stream_write_index + 1) % FORCE_STREAM_FRAMES;
    ++g_stream_count;
    g_force_table[FORCE_STREAM_TABLE_BASE]     = (TYPE_TABLE)g_stream_count;
    g_force_table[FORCE_STREAM_TABLE_BASE + 1] = (TYPE_TABLE)g_stream_write_index;

    return (int)g_stream_count;
}

// 力输入直接使用 ForceSample 采集的 g_fz_raw（有符号原始码，本周期同源）
int ForceCtrlMgr(void)
{
    fp32 f_sensor_flt;
    fp32 f_remove_tool;
    fp32 phi;
    fp32 theta;
    fp32 dz_sensor;
    fp32 v_max;

    if(g_initialized == 0 || g_force_table == 0 || g_force_params.adj_max <= 0.0f)
    {
        return FORCE_SENSOR_INVALID;
    }

    // 原始码先统一换算为N；过力保护检查传感器实际承受的力

    // 目前没有加入滤波=====================================================================================
    f_sensor_flt = g_fz_raw * FORCE_SENSOR_SCALE;
    if(fabs(f_sensor_flt) > FORCE_MAX_FORCE)
    {
        CtrlClear();
        return FORCE_SENSOR_OVERLIMIT;
    }

    // 直接读取虚拟轴 DPOS：轴 6-10 的 UNITS=1，脉冲数与 BASIC 的 DPOS 数值一致；
    // 轴 9/10 单位为角秒，先化度再转弧度。不再经 BASIC 写 TABLE 433-437 中转。
    phi = radians((fp32)motionrt_getpuldpos(9) / 3600.0);
    theta = radians((fp32)motionrt_getpuldpos(10) / 3600.0);

    // 去除随姿态变化的末端工具重力，g_tool为水平放置时的标定值
    f_remove_tool = f_sensor_flt - g_tool * cos(theta);

    if(g_force_state.flag_contacted == 0)
    {
        v_max = FORCE_V_MAX_SEARCH;

        // 接触前以接触阈值为搜索参考力
        g_force_state.f_ref_effective = g_force_params.contact_threshold;

        // 连续多个周期超过阈值才确认接触，避免瞬时波动误触发
        if(f_remove_tool > g_force_params.contact_threshold)
        {
            ++g_force_state.contact_count;
        }
        else
        {
            g_force_state.contact_count = 0;
        }

        if(g_force_state.contact_count >= FORCE_CONTACT_TICKS_REQUIRED)
        {
            g_force_state.flag_contacted = 1;

            // 接触瞬间从当前接触力开始加载，避免参考力突变
            g_force_state.f_ref_effective = f_remove_tool;
        }
    }
    else
    {
        v_max = FORCE_V_MAX_HOLD;

        // 双向斜坡：目标力增大或减小时均限制参考力变化量
        if(g_force_state.f_ref_effective < g_force_params.f_target)
        {
            g_force_state.f_ref_effective += g_force_params.ramp_step;
            if(g_force_state.f_ref_effective > g_force_params.f_target)
            {
                g_force_state.f_ref_effective = g_force_params.f_target;
            }
        }
        else if(g_force_state.f_ref_effective > g_force_params.f_target)
        {
            g_force_state.f_ref_effective -= g_force_params.ramp_step;
            if(g_force_state.f_ref_effective < g_force_params.f_target)
            {
                g_force_state.f_ref_effective = g_force_params.f_target;
            }
        }
    }

    dz_sensor = ImpedanceCtrl(f_remove_tool, g_force_state.f_ref_effective, v_max);

    // 将传感器坐标系调整量变换到基坐标系，并直接写回BASIC
    g_force_table[FORCE_TABLE_ADJ_BEGIN] = sin(theta) * cos(phi) * dz_sensor;
    g_force_table[FORCE_TABLE_ADJ_BEGIN + 1] = sin(theta) * sin(phi) * dz_sensor;
    g_force_table[FORCE_TABLE_ADJ_BEGIN + 2] = cos(theta) * dz_sensor;
    g_force_table[FORCE_TABLE_ADJ_BEGIN + 3] = 0.0;
    g_force_table[FORCE_TABLE_ADJ_BEGIN + 4] = 0.0;

    return FORCE_SENSOR_VALID;
}
