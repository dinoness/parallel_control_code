#pragma once

#include "../zmotion/ZMotionDriver.h"

/// @brief 控制器协议总入口预留类。
/// 当前实际协议由 CommandProtocol / JointProtocol / CartJogProtocol / TraceProtocol 分别实现。
class ControllerProtocol
{
public:
    explicit ControllerProtocol(ZMotionDriver* driver);

private:
    ZMotionDriver* driver_;
};
