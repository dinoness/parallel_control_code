#include "TrajectorySendWorker.h"

#include <QThread>
#include <QtGlobal>

TrajectorySendWorker::TrajectorySendWorker(TraceProtocol* protocol,
                                           QObject* parent)
    : QObject(parent),
      protocol_(protocol),
      cancelRequested_(false),
      paused_(false)
{
}

void TrajectorySendWorker::cancel()
{
    cancelRequested_.store(true);
    // 唤醒可能因 pause 而阻塞的循环
    paused_.store(false);
}

void TrajectorySendWorker::pause()
{
    paused_.store(true);
}

void TrajectorySendWorker::resume()
{
    paused_.store(false);
}

void TrajectorySendWorker::startSend(const QString& datFilePath, int totalPoints,
                                     int eventId)
{
    cancelRequested_.store(false);
    paused_.store(false);

    if (protocol_ == nullptr) {
        emit finished(Result::fail(3201, "TraceProtocol 未初始化"));
        return;
    }

    TrajectorySendOptions options;

    options.isCancelled = [this]() {
        return cancelRequested_.load();
    };

    options.isPaused = [this]() {
        return paused_.load();
    };

    options.onProgress = [this](int sentGroups, int totalGroups) {
        int percent = 0;
        if (totalGroups > 0) {
            percent = qBound(0, sentGroups * 100 / totalGroups, 100);
        }

        emit progressChanged(sentGroups, totalGroups, percent);
    };

    Result ret = protocol_->sendTrajectory(datFilePath, totalPoints, options, eventId);

    emit finished(ret);
}
