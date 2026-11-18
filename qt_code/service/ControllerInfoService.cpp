#include "ControllerInfoService.h"
#include <QDebug>
#include <QMetaObject>

ControllerInfoService::ControllerInfoService(ControllerInfoProtocol* protocol,
                                             QObject* parent)
    : QObject(parent), protocol_(protocol)
{
}

ControllerInfoService::~ControllerInfoService()
{
    stopAll();
}

Result ControllerInfoService::ensureThread()
{
    if (monitorThread_ != nullptr && worker_ != nullptr) return Result::success();

    monitorThread_ = new QThread();
    worker_ = new ControllerInfoWorker(protocol_);
    worker_->moveToThread(monitorThread_);

    connect(worker_, &ControllerInfoWorker::stateUpdated,
            this, &ControllerInfoService::stateUpdated, Qt::QueuedConnection);
    connect(worker_, &ControllerInfoWorker::sensorBatchReceived,
            this, &ControllerInfoService::sensorBatchReceived, Qt::QueuedConnection);
    connect(worker_, &ControllerInfoWorker::statusBatchReceived,
            this, &ControllerInfoService::statusBatchReceived, Qt::QueuedConnection);
    connect(worker_, &ControllerInfoWorker::forceStreamBatchReceived,
            this, &ControllerInfoService::forceStreamBatchReceived, Qt::QueuedConnection);
    connect(worker_, &ControllerInfoWorker::monitorError,
            this, &ControllerInfoService::monitorError, Qt::QueuedConnection);
    connect(monitorThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(monitorThread_, &QThread::finished, monitorThread_, &QObject::deleteLater);
    connect(monitorThread_, &QThread::finished, this, [this]() {
        worker_ = nullptr;
        monitorThread_ = nullptr;
    });

    monitorThread_->start();
    return Result::success();
}

void ControllerInfoService::cleanupThread()
{
    if (monitorThread_ == nullptr) return;
    monitorThread_->quit();
    if (!monitorThread_->wait(3000)) {
        qDebug() << "ControllerInfoService: monitor thread wait timeout";
        monitorThread_->terminate();
        monitorThread_->wait(1000);
    }
    worker_ = nullptr;
    monitorThread_ = nullptr;
}

Result ControllerInfoService::startStateMonitor(int intervalMs)
{
    Result ret = ensureThread();
    if (!ret.ok) return ret;
    bool invoked = QMetaObject::invokeMethod(worker_, [this, intervalMs]() {
        worker_->startStateMonitor(intervalMs);
    }, Qt::QueuedConnection);
    if (!invoked) return Result::fail(3105, "无法调用 Worker::startStateMonitor");
    stateMonitorRunning_ = true;
    return Result::success();
}

void ControllerInfoService::stopStateMonitor()
{
    stateMonitorRunning_ = false;
    if (worker_ != nullptr) {
        QMetaObject::invokeMethod(worker_, [this]() {
            worker_->stopStateMonitor();
        }, Qt::QueuedConnection);
    }
}

Result ControllerInfoService::startSensorUpload(const SensorTableConfig& config)
{
    Result ret = ensureThread();
    if (!ret.ok) return ret;
    bool invoked = QMetaObject::invokeMethod(worker_, [this, config]() {
        worker_->startSensorUpload(config);
    }, Qt::QueuedConnection);
    if (!invoked) return Result::fail(3106, "无法调用 Worker::startSensorUpload");
    sensorUploadRunning_ = true;
    return Result::success();
}

void ControllerInfoService::stopSensorUpload()
{
    sensorUploadRunning_ = false;
    if (worker_ != nullptr) {
        QMetaObject::invokeMethod(worker_, [this]() {
            worker_->stopSensorUpload();
        }, Qt::QueuedConnection);
    }
}

Result ControllerInfoService::startStatusMonitor(int intervalMs)
{
    Result ret = ensureThread();
    if (!ret.ok) return ret;
    bool invoked = QMetaObject::invokeMethod(worker_, [this, intervalMs]() {
        worker_->startStatusMonitor(intervalMs);
    }, Qt::QueuedConnection);
    if (!invoked) return Result::fail(3104, "无法调用 Worker::startStatusMonitor");
    statusMonitorRunning_ = true;
    return Result::success();
}

void ControllerInfoService::stopStatusMonitor()
{
    statusMonitorRunning_ = false;
    if (worker_ != nullptr) {
        QMetaObject::invokeMethod(worker_, [this]() {
            worker_->stopStatusMonitor();
        }, Qt::QueuedConnection);
    }
}

Result ControllerInfoService::startForceStreamMonitor(int intervalMs)
{
    Result ret = ensureThread();
    if (!ret.ok) return ret;
    bool invoked = QMetaObject::invokeMethod(worker_, [this, intervalMs]() {
        worker_->startForceStreamMonitor(intervalMs);
    }, Qt::QueuedConnection);
    if (!invoked) return Result::fail(3107, "无法调用 Worker::startForceStreamMonitor");
    forceStreamMonitorRunning_ = true;
    return Result::success();
}

void ControllerInfoService::stopForceStreamMonitor()
{
    forceStreamMonitorRunning_ = false;
    if (worker_ != nullptr) {
        QMetaObject::invokeMethod(worker_, [this]() {
            worker_->stopForceStreamMonitor();
        }, Qt::QueuedConnection);
    }
}

void ControllerInfoService::stopAll()
{
    stopStateMonitor();
    stopSensorUpload();
    stopStatusMonitor();
    stopForceStreamMonitor();
    cleanupThread();
}

bool ControllerInfoService::isStateMonitorRunning() const
{
    return stateMonitorRunning_;
}

bool ControllerInfoService::isSensorUploadRunning() const
{
    return sensorUploadRunning_;
}

bool ControllerInfoService::isStatusMonitorRunning() const
{
    return statusMonitorRunning_;
}

bool ControllerInfoService::isForceStreamMonitorRunning() const
{
    return forceStreamMonitorRunning_;
}
