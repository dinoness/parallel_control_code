#pragma once

#include <QObject>

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../protocol/CartJogProtocol.h"
#include "../protocol/CommandProtocol.h"
#include "../protocol/JointProtocol.h"
#include "../protocol/TraceProtocol.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 运动控制 Service — 协调 Direct Joint / Cart Jog / Home / Trace 等运动模式
class MotionService : public QObject
{
    Q_OBJECT

public:
    explicit MotionService(ZMotionDriver* driver,
                           CommandProtocol* commandProtocol,
                           TraceProtocol* traceProtocol,
                           QObject* parent = nullptr);

    // ── 全局控制 ──────────────────────────────────────
    Result sendHome();
    Result sendPause();
    Result sendResume();
    Result sendStop();
    Result sendEstop();
    Result sendErrorReset();

    // ── Joint 模式 ────────────────────────────────────
    Result enterJointMode();
    Result exitJointMode();
    Result sendDirectJoint(float j1, float j2, float j3, float j4, float j5,
                           int speedLevel);

    // ── Cart Jog 模式 ─────────────────────────────────
    Result enterCartJogMode();
    Result exitCartJogMode();
    Result sendCartJog(int cmdId, float x, float y, float z, float phi,
                       float theta, int speedLevel);

    // ── Trace 模式 ────────────────────────────────────
    Result enterTraceMode();
    Result exitTraceMode();
    Result canExitTraceMode();

    // ── Robot Mode ────────────────────────────────────
    Result enterRobotMode();
    Result exitRobotMode();

private:
    ZMotionDriver*   driver_;
    CommandProtocol* commandProtocol_;
    JointProtocol    jointProtocol_;
    CartJogProtocol  cartJogProtocol_;
    TraceProtocol*   traceProtocol_;
};
