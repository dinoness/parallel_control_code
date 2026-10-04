#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <atomic>

#include "../core/Result.h"
#include "../protocol/TraceProtocol.h"

/// @brief 轨迹下发后台 Worker — 运行在独立 QThread 中
///
/// 负责阻塞式等待缓冲区、流式读文件、写入 TABLE、发事件、报告进度、响应取消/暂停。
/// 禁止直接访问 UI 控件。
class TrajectorySendWorker : public QObject
{
    Q_OBJECT

public:
    explicit TrajectorySendWorker(TraceProtocol* protocol,
                                  QObject* parent = nullptr);

    bool isPaused() const { return paused_.load(); }

public slots:
    /// @brief 开始下发，eventId 为启动事件 ID（kEventTraj / kEventCtrl）
    void startSend(const QString& datFilePath, int totalPoints, int eventId);
    void cancel();
    void pause();
    void resume();

signals:
    void progressChanged(int sentGroups, int totalGroups, int percent);
    void finished(Result result);

private:
    TraceProtocol* protocol_ = nullptr;
    std::atomic_bool cancelRequested_;
    std::atomic_bool paused_;
};
