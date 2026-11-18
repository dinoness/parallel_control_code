#include "zmcbuildin.h" 

int ctrl_int(void)
{
    // 控制频率
    // m, b, k
    // e0, e1, e2
    // 常数寄存器位置
    return 1;

}

fp32 impendance_ctrl(fp32 f_sensor, fp32 f_ref)
{
    static fp32 dz_sum;
    fp32 dz;
    fp32 f_err = f_sensor - f_ref;
    static fp32 e2;
    static fp32 e1;
    fp32 e0;
    fp32 m = 2.5;
    fp32 b = 80;
    fp32 k = 1000;
    fp32 T = 0.001;

    e0 = (pow(T, 2) * f_err + (T*b + 2*m) * e1 - m * e2) / (m + T*b + pow(T, 2) * k);
    e2 = e1;
    e1 = e0;

    
    dz = e0*1000000;
    // if(abs(dz) > 30)
    // {
    //     dz = dz/abs(dz)*30;
    // }

    // dz_sum = dz_sum + dz;
    // if(abs(dz_sum) > 3000)
    // {
    //     return dz_sum = dz_sum/abs(dz_sum)*3000;
    // }
    // else
    // {
    //     return dz_sum;
    // }

    if (fabs(dz) > 10.0)                      // 修正速度限幅 ~2 mm/s，与虚轴 SPEED 匹配
        dz = (dz > 0) ? 10.0 : -10.0;

    dz_sum = dz_sum + dz;
    if (fabs(dz_sum) > 5000.0)               // 保持在边界，不回跳
        dz_sum = (dz_sum > 0) ? 5000.0 : -5000.0;
    return dz_sum;


}





