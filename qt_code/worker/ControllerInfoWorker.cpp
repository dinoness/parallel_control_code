#include "ControllerInfoWorker.h"
#include <QDebug>

ControllerInfoWorker::ControllerInfoWorker(ControllerInfoProtocol* protocol,
                                           QObject* parent)
    : QObject(parent),
      protocol_(protocol),
      stateTimer_(new QTimer(this)),
      sensorTimer_(new QTimer(this)),
      statusTimer_(new QTimer(this))
{
    stateTimer_->setSingleShot(false);
    sensorTimer_->setSingleShot(false);
    statusTimer_->setSingleShot(false);
}

void ControllerInfoWorker::startStateMonitor(int intervalMs)
{
    if (protocol_ == nullptr) return;

    // 立即读取一次状态
    pollStateOnce();

    stateTimer_->setInterval(intervalMs);
    connect(stateTimer_, &QTimer::timeout,
            this, &ControllerInfoWorker::pollStateOnce,
            Qt::UniqueConnection);

    if (!stateTimer_->isActive()) {
        stateTimer_->start();
    }
}

void ControllerInfoWorker::stopStateMonitor()
{
    if (stateTimer_->isActive()) {
        stateTimer_->stop();
        stateTimer_->disconnect(this);
    }
}

void ControllerInfoWorker::startSensorUpload(const SensorTableConfig& config)
{
    if (protocol_ == nullptr) return;

    sensorConfig_ = config;
    lastSensorReadIndex_ = 0;
    lastSensorFrameCounter_ = 0;
    sensorUploadRunning_ = true;

    // 立即读取一次
    pollSensorOnce();

    sensorTimer_->setInterval(config.uploadIntervalMs);
    connect(sensorTimer_, &QTimer::timeout,
            this, &ControllerInfoWorker::pollSensorOnce,
            Qt::UniqueConnection);

    if (!sensorTimer_->isActive()) {
        sensorTimer_->start();
    }
}

void ControllerInfoWorker::stopSensorUpload()
{
    sensorUploadRunning_ = false;

    if (sensorTimer_->isActive()) {
        sensorTimer_->stop();
        sensorTimer_->disconnect(this);
    }
}

void ControllerInfoWorker::startStatusMonitor(int intervalMs)
{
    if (protocol_ == nullptr) return;

    lastStatusFrameCounter_ = 0;

    // 立即读取一次状态数据
    pollStatusOnce();

    statusTimer_->setInterval(intervalMs);
    connect(statusTimer_, &QTimer::timeout,
            this, &ControllerInfoWorker::pollStatusOnce,
            Qt::UniqueConnection);

    if (!statusTimer_->isActive()) {
        statusTimer_->start();
    }
}

void ControllerInfoWorker::stopStatusMonitor()
{
    lastStatusFrameCounter_ = 0;

    if (statusTimer_->isActive()) {
        statusTimer_->stop();
        statusTimer_->disconnect(this);
    }
}

void ControllerInfoWorker::pollStateOnce()
{
    if (protocol_ == nullptr) return;

    ControllerStateSnapshot snapshot;
    Result ret = protocol_->readSystemState(snapshot);

    if (!ret.ok) {
        emit monitorError(ret);
        return;
    }

    emit stateUpdated(snapshot);
}

void ControllerInfoWorker::pollSensorOnce()
{
    if (protocol_ == nullptr || !sensorUploadRunning_) return;

    SensorTableBatch batch;
    Result ret = protocol_->readSensorBatch(sensorConfig_,
                                            lastSensorReadIndex_,
                                            lastSensorFrameCounter_,
                                            batch);

    if (!ret.ok) {
        emit monitorError(ret);
        return;
    }

    if (!batch.frames.isEmpty()) {
        emit sensorBatchReceived(batch);
    }
}

void ControllerInfoWorker::pollStatusOnce()
{
    if (protocol_ == nullptr) return;

    StatusTableBatch batch;
    Result ret = protocol_->readStatusBatch(lastStatusFrameCounter_, batch);

    if (!ret.ok) {
        emit monitorError(ret);
        return;
    }

    if (!batch.frames.isEmpty()) {
        emit statusBatchReceived(batch);
    }
}
