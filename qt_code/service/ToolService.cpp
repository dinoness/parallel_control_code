#include "ToolService.h"

ToolService::ToolService(ZMotionDriver* driver, QObject* parent)
    : QObject(parent)
    , toolProtocol_(driver)
{
}

Result ToolService::startRevolution(float rpm)
{
    return toolProtocol_.startRevolution(rpm);
}

Result ToolService::stopRevolution()
{
    return toolProtocol_.stopRevolution();
}

Result ToolService::startRotation(float rpm)
{
    return toolProtocol_.startRotation(rpm);
}

Result ToolService::stopRotation()
{
    return toolProtocol_.stopRotation();
}
