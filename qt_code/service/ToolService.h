#pragma once

#include <QObject>

#include "../core/Result.h"
#include "../protocol/ToolProtocol.h"
#include "../zmotion/ZMotionDriver.h"

/// @brief 末端工具服务 — 向 UI 提供公转/自转独立启停接口
class ToolService : public QObject
{
    Q_OBJECT

public:
    explicit ToolService(ZMotionDriver* driver, QObject* parent = nullptr);

    Result startRevolution(float rpm);
    Result stopRevolution();
    Result startRotation(float rpm);
    Result stopRotation();

private:
    ToolProtocol toolProtocol_;
};
