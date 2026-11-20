#pragma once

#include <stdint.h>
#include <QMetaType>

#include "../core/ProtocolConstants.h"

struct TrajectoryPoint {
    float cmd;
    float x;
    float y;
    float z;
    float theta;
    float phi;
    float ticks;

    TrajectoryPoint()
        : cmd(0), x(0), y(0), z(0), theta(0), phi(0), ticks(0) {}

    // CTRL 力控段配置。mode 取 kForceModeSearch / kForceModeHold；
    // 普通坐标字段在此记录中按协议复用为力控参数，rampStepN 单位为 N/控制周期。
    static TrajectoryPoint forceConfig(int mode,
                                       float targetForceN,
                                       float contactThresholdN,
                                       float rampStepN,
                                       float maxAdjustUm) {
        TrajectoryPoint p;
        p.cmd = kCmdForceCfg;
        p.x = static_cast<float>(mode);
        p.y = targetForceN;
        p.z = contactThresholdN;
        p.theta = rampStepN;
        p.phi = maxAdjustUm;
        return p;
    }

    static TrajectoryPoint forceOff() {
        TrajectoryPoint p;
        p.cmd = kCmdForceOff;
        return p;
    }

    void toArray(float arr[kTrajCmdSize]) const {
        arr[0] = cmd;
        arr[1] = x;
        arr[2] = y;
        arr[3] = z;
        arr[4] = theta;
        arr[5] = phi;
        arr[6] = ticks;
    }

    static TrajectoryPoint fromArray(const float arr[kTrajCmdSize]) {
        TrajectoryPoint p;
        p.cmd = arr[0];
        p.x = arr[1];
        p.y = arr[2];
        p.z = arr[3];
        p.theta = arr[4];
        p.phi = arr[5];
        p.ticks = arr[6];
        return p;
    }
};

Q_DECLARE_METATYPE(TrajectoryPoint)
