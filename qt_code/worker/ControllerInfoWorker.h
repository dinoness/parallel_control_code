#pragma once

#include <QObject>
#include <QTimer>
#include <atomic>

#include "../core/ControllerInfoTypes.h"
#include "../core/Result.h"
#include "../protocol/ControllerInfoProtocol.h"

/// @brief 控制器信息后台 Worker — 运行在独立 QThread 中
///
/// 内部使用两个 QTimer：
/// - stateTimer_：默认 200 ms，读取系统状态
/// - sensorTimer_：默认不启动，调用 startSensorUpload() 后启动
///
/// 注意：控制器状态显示周期为 200 ms，属于低频监控。
/// 传感器信号由控制器端每 1 ms 采样写入 TABLE 环形缓冲，
/// 上位机不做 1 ms 单点读取，而是批量读取。
/// 所有 ZAux API 调用通过 ZMotionDriver 的 mutex 串行化，
/// 不会因 TABLE 批量读取而阻塞急停或轨迹下发过久。
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

signals:
    void stateUpdated(const ControllerStateSnapshot& snapshot);
    void sensorBatchReceived(const SensorTableBatch& batch);
    void monitorError(Result result);

private slots:
    void pollStateOnce();
    void pollSensorOnce();

private:
    ControllerInfoProtocol* protocol_ = nullptr;

    QTimer* stateTimer_ = nullptr;
    QTimer* sensorTimer_ = nullptr;

    SensorTableConfig sensorConfig_;
    int lastSensorReadIndex_ = 0;
    quint64 lastSensorFrameCounter_ = 0;
    bool sensorUploadRunning_ = false;
};
