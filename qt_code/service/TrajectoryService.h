#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>

#include "../core/Result.h"
#include "../motion/TrajectoryFile.h"
#include "../motion/TrajectoryTypes.h"
#include "../protocol/TraceProtocol.h"
#include "../zmotion/ZMotionDriver.h"
#include "../worker/TrajectorySendWorker.h"

class TrajectoryService : public QObject
{
    Q_OBJECT

public:
    explicit TrajectoryService(ZMotionDriver* driver,
                               TraceProtocol* protocol,
                               const QString& dataDir,
                               QObject* parent = nullptr);
    ~TrajectoryService();

    // ── 文件操作 ──────────────────────────────────────
    Result loadPoints(const QString& fileName, QVector<TrajectoryPoint>& points);
    Result datToCsv(const QString& datFile, const QString& csvFile);
    Result csvToDat(const QString& csvFile, const QString& datFile);

    // ── 异步下发（UI 调用） ────────────────────────────
    /// @brief 流式下发轨迹文件：Worker 中直接读 .dat 边读边发
    /// @param eventId 启动事件 ID，Trace 模式默认 kEventTraj，
    ///                轨迹控制 (Ctrl Trace) 模式传 kEventCtrl
    Result startSendTrajectoryAsync(const QString& fileName, int eventId = kEventTraj);
    /// @brief 暂停轨迹下发：设置 Worker 暂停标志，保留进度
    void   pauseSendTrajectory();
    /// @brief 恢复轨迹下发：清除 Worker 暂停标志，继续下发
    void   resumeSendTrajectory();
    void   cancelSendTrajectory();
    /// @brief 停止下发线程，带超时等待 (ms)，返回 true 表示线程已结束
    bool   stopSendThread(int timeoutMs = 5000);
    bool   isSending() const;
    bool   isPaused() const;

signals:
    void sendProgressChanged(int sentGroups, int totalGroups, int percent);
    void sendFinished(Result result);
    void sendingStateChanged(bool sending);

private:
    void cleanupSendThread();

    ZMotionDriver*           driver_   = nullptr;
    TraceProtocol*           protocol_ = nullptr;
    TrajectoryFile           file_;

    QThread*                 sendThread_ = nullptr;
    TrajectorySendWorker*    sendWorker_ = nullptr;
    bool                     sending_    = false;
};
