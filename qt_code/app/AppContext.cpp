#include "AppContext.h"

AppContext::AppContext()
    : driver_(),
      protocol_(&driver_),
      commandProtocol_(&driver_),
      traceProtocol_(&driver_),
      controllerInfoProtocol_(&driver_),
      connectionService_(&driver_),
      motionService_(&driver_, &commandProtocol_, &traceProtocol_),
      trajectoryService_(&driver_, &traceProtocol_, "../../trace_data/"),
      controllerInfoService_(&controllerInfoProtocol_)
{
}

ZMotionDriver* AppContext::driver()
{
    return &driver_;
}

ControllerProtocol* AppContext::protocol()
{
    return &protocol_;
}

CommandProtocol* AppContext::commandProtocol()
{
    return &commandProtocol_;
}

TraceProtocol* AppContext::traceProtocol()
{
    return &traceProtocol_;
}

ConnectionService* AppContext::connectionService()
{
    return &connectionService_;
}

MotionService* AppContext::motionService()
{
    return &motionService_;
}

TrajectoryService* AppContext::trajectoryService()
{
    return &trajectoryService_;
}

ControllerInfoProtocol* AppContext::controllerInfoProtocol()
{
    return &controllerInfoProtocol_;
}

ControllerInfoService* AppContext::controllerInfoService()
{
    return &controllerInfoService_;
}
