#include "TrajectoryService.h"

#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>

TrajectoryService::TrajectoryService(ZMotionDriver* driver,
                                     TraceProtocol* protocol,
                                     const QString& dataDir,
                                     QObject* parent)
    : QObject(parent),
      driver_(driver),
      protocol_(protocol),
      file_(dataDir)
{
}

TrajectoryService::~TrajectoryService()
{
    cancelSendTrajectory();

    if (sendThread_ != nullptr) {
        sendThread_->quit();
        sendThread_->wait(3000);
    }

    cleanupSendThread();
}

bool TrajectoryService::isSending() const
{
    return sending_;
}

// ── 文件操作 ──────────────────────────────────────────────────────

Result TrajectoryService::loadPoints(const QString& fileName,
                                      QVector<TrajectoryPoint>& points)
{
    return file_.readDat(fileName, points);
}

Result TrajectoryService::datToCsv(const QString& datFile, const QString& csvFile)
{
    QVector<TrajectoryPoint> points;
    Result ret = file_.readDat(datFile, points);
    if (!ret.ok) return ret;

    return file_.writeCsv(csvFile, points);
}

Result TrajectoryService::csvToDat(const QString& csvFile, const QString& datFile)
{
    QVector<TrajectoryPoint> points;
    Result ret = file_.readCsv(csvFile, points);
    if (!ret.ok) return ret;

    return file_.writeDat(datFile, points);
}

// ── 异步下发 ──────────────────────────────────────────────────────

Result TrajectoryService::startSendTrajectoryAsync(const QString& fileName, int eventId)
{
    if (protocol_ == nullptr) {
        return Result::fail(3301, "TraceProtocol 未初始化");
    }

    if (sending_ || sendThread_ != nullptr || sendWorker_ != nullptr) {
        return Result::fail(3303, "当前已有轨迹正在下发");
    }

    // 构建完整 .dat 文件路径
    QString datFilePath = file_.datPath(fileName);

    // .dat 为无文件头的纯 float32 流，每点固定 kTrajCmdSize 个 float，
    // 直接按文件大小计算总点数（用于进度计算），
    // 实际下发时 Protocol 层会打开文件流式读取
    QFileInfo fileInfo(datFilePath);
    if (!fileInfo.exists()) {
        return Result::fail(3302, QString("轨迹文件不存在：%1").arg(datFilePath));
    }

    int totalPoints = static_cast<int>(fileInfo.size() / (kTrajCmdSize * sizeof(float)));
    if (totalPoints <= 0) {
        return Result::fail(3302, "轨迹数据为空，无法下发");
    }

    sending_ = true;
    emit sendingStateChanged(true);

    sendThread_ = new QThread();
    sendWorker_ = new TrajectorySendWorker(protocol_);

    sendWorker_->moveToThread(sendThread_);

    connect(sendThread_, &QThread::started,
            sendWorker_, [this, datFilePath, totalPoints, eventId]() {
                sendWorker_->startSend(datFilePath, totalPoints, eventId);
            });

    connect(sendWorker_, &TrajectorySendWorker::progressChanged,
            this, &TrajectoryService::sendProgressChanged);

    connect(sendWorker_, &TrajectorySendWorker::finished,
            this, [this](const Result& result) {
                emit sendFinished(result);

                if (sendThread_ != nullptr) {
                    sendThread_->quit();
                }
            });

    connect(sendThread_, &QThread::finished,
            sendWorker_, &QObject::deleteLater);

    connect(sendThread_, &QThread::finished,
            sendThread_, &QObject::deleteLater);

    connect(sendThread_, &QThread::finished,
            this, [this]() {
                sendWorker_ = nullptr;
                sendThread_ = nullptr;
                sending_ = false;
                emit sendingStateChanged(false);
            });

    sendThread_->start();

    return Result::success();
}

void TrajectoryService::cancelSendTrajectory()
{
    if (sendWorker_ != nullptr) {
        sendWorker_->cancel();
    }
}

void TrajectoryService::pauseSendTrajectory()
{
    if (sendWorker_ != nullptr) {
        sendWorker_->pause();
    }
}

void TrajectoryService::resumeSendTrajectory()
{
    if (sendWorker_ != nullptr) {
        sendWorker_->resume();
    }
}

bool TrajectoryService::isPaused() const
{
    return sendWorker_ != nullptr && sendWorker_->isPaused();
}

bool TrajectoryService::stopSendThread(int timeoutMs)
{
    if (sendThread_ == nullptr && sendWorker_ == nullptr) {
        return true;
    }

    cancelSendTrajectory();

    if (sendThread_ != nullptr) {
        sendThread_->quit();

        if (!sendThread_->wait(timeoutMs)) {
            qDebug() << "TrajectoryService::stopSendThread: timeout waiting for worker to stop";
            return false;
        }
    }

    return true;
}

void TrajectoryService::cleanupSendThread()
{
    sendWorker_ = nullptr;
    sendThread_ = nullptr;
    sending_ = false;
}
