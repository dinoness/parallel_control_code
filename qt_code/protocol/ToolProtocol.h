#pragma once

#include "../core/ProtocolConstants.h"
#include "../core/Result.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 末端工具协议 — 公转/自转电机转速下发与独立启停
class ToolProtocol
{
public:
    explicit ToolProtocol(ZMotionDriver* driver);

    Result startRevolution(float rpm);
    Result stopRevolution();
    Result startRotation(float rpm);
    Result stopRotation();

private:
    Result startTool(int tableAddr, float rpm, int startEvent);
    Result stopTool(int stopEvent);

    ZMotionDriver* driver_;
};
