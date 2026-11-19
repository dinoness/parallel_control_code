#include "zmcbuildin.h" 
#define FORCE_MODE_SEARCH 1
#define FORCE_MODE_HOLD 2

int ctrl_int(void)
{
    // 控制频率
    // m, b, k
    // e0, e1, e2
    // 常数寄存器位置
    // 创建一个信号结构体，分配空间指针
    return 0;

}

fp32 impendanceCtrl(fp32 f_sensor, fp32 f_ref)
{
    fp32 f_err = f_sensor - f_ref;
    static fp32 e2;
    static fp32 e1;
    fp32 e0;
    fp32 delta_e;
    fp32 m = 2.5;
    fp32 b = 80;
    fp32 k = 1000;
    fp32 T = 0.001;

    e0 = (pow(T, 2) * f_err + (T*b + 2*m) * e1 - m * e2) / (m + T*b + pow(T, 2) * k);
    delta_e = e0 - e1;
    
    if(fabs(delta_e) > 1e-5)  // 此处单位为m
        e0 = e1 + ((delta_e > 0) ? 1e-5 : -1e-5);
    
    e2 = e1;
    e1 = e0;

    if(fabs(e0) > 1e-2)
        e0 = (e0 > 0) ? 1e-2 : -1e-2;

    return e0*1e6;
}


void ForceCtrlMgr(uint8 force_state, fp32 f_sensor, fp32 f_ref)
{
    static fp32 dz_sensor;
    fp32 phi;
    fp32 theta;
    fp32 g_tool;
    fp32 f_sensor_flt;
    fp32 f_remove_tool;
    fp32 p_adj[5];

    // 力信号滤波


    // 去重力
    f_remove_tool = f_sensor_flt + g_tool * cos(theta);


    if(force_state == FORCE_MODE_SEARCH)
    {
        // 接触到力>1N之后，进行斜坡加载参考力
    }

    dz_sensor = impendanceCtrl(f_sensor, f_ref);

    // 将调整量从传感器坐标系变换到基坐标系
    p_adj[0] = sin(theta) * cos(phi) * dz_sensor;
    p_adj[1] = sin(theta) * sin(phi) * dz_sensor;
    p_adj[2] = cos(theta) * dz_sensor;

}



