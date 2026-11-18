#pragma once

#include <QObject>
#include <QThread>

#include "../core/ControllerInfoTypes.h"
#include "../core/Result.h"
#include "../protocol/ControllerInfoProtocol.h"
#include "../worker/ControllerInfoWorker.h"

/// @brief 控制器信息服务，统一管理后台监控线程生命周期
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

    Result startStatusMonitor(int intervalMs = kStatusUploadIntervalMs);
    void stopStatusMonitor();

    Result startForceStreamMonitor(int intervalMs = kForceStreamUploadIntervalMs);
    void stopForceStreamMonitor();

    void stopAll();

    bool isStateMonitorRunning() const;
    bool isSensorUploadRunning() const;
    bool isStatusMonitorRunning() const;
    bool isForceStreamMonitorRunning() const;

signals:
    void stateUpdated(const ControllerStateSnapshot& snapshot);
    void sensorBatchReceived(const SensorTableBatch& batch);
    void statusBatchReceived(const StatusTableBatch& batch);
    void forceStreamBatchReceived(const ForceStreamBatch& batch);
    void monitorError(Result result);

private:
    Result ensureThread();
    void cleanupThread();

    ControllerInfoProtocol* protocol_ = nullptr;
    QThread* monitorThread_ = nullptr;
    ControllerInfoWorker* worker_ = nullptr;

    bool stateMonitorRunning_ = false;
    bool sensorUploadRunning_ = false;
    bool statusMonitorRunning_ = false;
    bool forceStreamMonitorRunning_ = false;
};
