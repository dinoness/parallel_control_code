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

Result ToolService::startRotation(float rpm, int currentMa)
{
    return toolProtocol_.startRotation(rpm, currentMa);
}

Result ToolService::stopRotation()
{
    return toolProtocol_.stopRotation();
}
