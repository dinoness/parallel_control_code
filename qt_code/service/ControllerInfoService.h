#pragma once

#include <QObject>
#include <QThread>

#include "../core/ControllerInfoTypes.h"
#include "../core/Result.h"
#include "../protocol/ControllerInfoProtocol.h"
#include "../worker/ControllerInfoWorker.h"

/// @brief 控制器信息服务 — 管理监控线程生命周期
///
/// 负责创建 QThread 和 ControllerInfoWorker，提供 start/stop 接口。
/// 断开控制器连接前，必须调用 stopAll() 避免 Worker 继续读取已关闭的 handle。
class ControllerInfoService : public QObject
{
    Q_OBJECT

public:
    explicit ControllerInfoService(ControllerInfoProtocol* protocol,
                                   QObject* parent = nullptr);
    ~ControllerInfoService();

    Result startStateMonitor(int intervalMs = 200);
    void stopStateMonitor();

    Result startSensorUpload(const SensorTableConfig& config = SensorTableConfig());
    void stopSensorUpload();

    void stopAll();

    bool isStateMonitorRunning() const;
    bool isSensorUploadRunning() const;

signals:
    void stateUpdated(const ControllerStateSnapshot& snapshot);
    void sensorBatchReceived(const SensorTableBatch& batch);
    void monitorError(Result result);

private:
    Result ensureThread();
    void cleanupThread();

    ControllerInfoProtocol* protocol_ = nullptr;
    QThread* monitorThread_ = nullptr;
    ControllerInfoWorker* worker_ = nullptr;

    bool stateMonitorRunning_ = false;
    bool sensorUploadRunning_ = false;
};
