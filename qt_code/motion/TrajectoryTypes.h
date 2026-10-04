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
