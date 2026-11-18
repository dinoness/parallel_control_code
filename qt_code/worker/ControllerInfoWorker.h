#pragma once

#include <QObject>
#include <QTimer>
#include <atomic>

#include "../core/ControllerInfoTypes.h"
#include "../core/Result.h"
#include "../protocol/ControllerInfoProtocol.h"

/// @brief 控制器信息后台Worker，所有ZAux调用均在此线程执行
class ControllerInfoWorker : public QObject
{
    Q_OBJECT

public:
    explicit ControllerInfoWorker(ControllerInfoProtocol* protocol,
                                  QObject* parent = nullptr);

public slots:
    void startStateMonitor(int intervalMs);
    void stopStateMonitor();

    void startSensorUpload(const SensorTableConfig& config);
    void stopSensorUpload();

    void startStatusMonitor(int intervalMs);
    void stopStatusMonitor();

    void startForceStreamMonitor(int intervalMs);
    void stopForceStreamMonitor();

signals:
    void stateUpdated(const ControllerStateSnapshot& snapshot);
    void sensorBatchReceived(const SensorTableBatch& batch);
    void statusBatchReceived(const StatusTableBatch& batch);
    void forceStreamBatchReceived(const ForceStreamBatch& batch);
    void monitorError(Result result);

private slots:
    void pollStateOnce();
    void pollSensorOnce();
    void pollStatusOnce();
    void pollForceStreamOnce();

private:
    ControllerInfoProtocol* protocol_ = nullptr;

    QTimer* stateTimer_ = nullptr;
    QTimer* sensorTimer_ = nullptr;
    QTimer* statusTimer_ = nullptr;
    QTimer* forceStreamTimer_ = nullptr;

    SensorTableConfig sensorConfig_;
    int lastSensorReadIndex_ = 0;
    quint64 lastSensorFrameCounter_ = 0;
    bool sensorUploadRunning_ = false;

    quint64 lastStatusFrameCounter_ = 0;
    quint64 lastForceStreamFrameCounter_ = 0;
};
