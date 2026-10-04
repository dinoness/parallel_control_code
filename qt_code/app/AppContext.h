#pragma once

#include "zmotion/ZMotionDriver.h"
#include "protocol/CommandProtocol.h"
#include "protocol/ControllerInfoProtocol.h"
#include "protocol/ControllerProtocol.h"
#include "protocol/TraceProtocol.h"
#include "service/ConnectionService.h"
#include "service/ControllerInfoService.h"
#include "service/MotionService.h"
#include "service/TrajectoryService.h"

class AppContext
{
public:
    AppContext();

    ZMotionDriver* driver();
    ControllerProtocol* protocol();
    CommandProtocol* commandProtocol();
    TraceProtocol* traceProtocol();
    ControllerInfoProtocol* controllerInfoProtocol();
    ConnectionService* connectionService();
    MotionService* motionService();
    TrajectoryService* trajectoryService();
    ControllerInfoService* controllerInfoService();

private:
    ZMotionDriver driver_;
    ControllerProtocol protocol_;
    CommandProtocol commandProtocol_;
    TraceProtocol traceProtocol_;
    ControllerInfoProtocol controllerInfoProtocol_;
    ConnectionService connectionService_;
    MotionService motionService_;
    TrajectoryService trajectoryService_;
    ControllerInfoService controllerInfoService_;
};
