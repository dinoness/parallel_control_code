#ifndef _FORCE_CONTROL_H
#define _FORCE_CONTROL_H

#include "zmcbuildin.h"
#include "myeigen.h"

// 生命周期函数返回值
#define FORCE_CTRL_OK                 0
#define FORCE_CTRL_ERROR             -1
#define FORCE_CTRL_PARAM_ERROR       -2

// 周期计算返回值，与BASIC中的传感器状态保持一致
#define FORCE_SENSOR_INVALID          2
#define FORCE_SENSOR_VALID            1
#define FORCE_SENSOR_OVERLIMIT        4

// 固定传感器与接触判定参数
#define FORCE_SENSOR_SCALE            (49.03325f / 32768.0f)
#define FORCE_MAX_FORCE               45.0f
#define FORCE_CONTACT_TICKS_REQUIRED  5

// 阻抗单周期位移限幅，单位m
#define FORCE_V_MAX_SEARCH            1e-6f
#define FORCE_V_MAX_HOLD              1e-5f

// 力控TABLE地址；g_tool保存水平放置时的Fz原始码
#define FORCE_TABLE_G_TOOL            427
#define FORCE_TABLE_ADJ_BEGIN         428
// 433-437 保留：位姿改由 C 侧 motionrt_getpuldpos 直接读取，不再经 TABLE 中转

// 以下地址与 global_config.bas 一一对应，任何一侧修改另一侧必须同步
// 力传感器适配参数区（TABLE_FORCE_CFG_BEGIN）
#define FORCE_TABLE_CFG_BEGIN         410
#define FORCE_CFG_ENABLE_OFFSET       0     // 410: 1=启用传感器采集
// 高频Fz独立环形区：header = 帧计数 + 写索引；数据先写、header 最后提交
#define FORCE_STREAM_TABLE_BASE       29450
#define FORCE_STREAM_DATA_BEGIN       29452
#define FORCE_STREAM_FRAMES           2048
// EL3702 CH1 模拟量输入编号（BASIC 侧 FORCE_AIN_BASE + 1）
#define FORCE_AIN_NUM                 33

typedef struct
{
    // 每段力控指令
    fp32 f_target;
    fp32 contact_threshold;
    fp32 ramp_step;       // N/控制周期
    fp32 adj_max;         // 总位移限幅，C内部统一为m
} ForceMgrParams;

typedef struct
{
    // 阻抗连续参数
    fp32 m;
    fp32 b;
    fp32 k;
    fp32 T;

    // 离散公式缓存系数
    fp32 c_f;
    fp32 c_e1;
    fp32 c_e2;

    // 阻抗算法离散历史
    fp32 e1;
    fp32 e2;
} ImpedanceParams;

typedef struct
{
    // 接触搜索与参考力斜坡状态
    fp32 f_ref_effective;
    uint16 contact_count;
    uint8 flag_contacted;
} ForceCtrlState;

int CtrlInit(void);
int CtrlPara(fp32 f_target, fp32 contact_threshold, fp32 ramp_step, fp32 max_adjust);
int CtrlClear(void);
int ForceSample(void);
int ForceCtrlMgr(void);

#endif
