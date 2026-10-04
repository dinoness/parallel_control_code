#include "TraceProtocol.h"
#include <QFile>
#include <QDataStream>
#include <QThread>
#include <QDebug>
#include <QtGlobal>

#include "../core/ProtocolConstants.h"

TraceProtocol::TraceProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

// ===================================================================
// 模式切换
// ===================================================================

Result TraceProtocol::enterTraceMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3402, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

Result TraceProtocol::canExitTrace()
{
    if (driver_ == nullptr) {
        return Result::fail(3403, "ZMotionDriver 未初始化");
    }

    uint16_t state = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, state);
    if (!ret.ok) return ret;

    if (state == kSysRunning) {
        return Result::fail(3404,
            QString("控制器正在运行轨迹 (状态寄存器 %1 = SYS_RUNNING)，请先停止轨迹")
                .arg(kRegSystemState));
    }

    return Result::success();
}

Result TraceProtocol::exitTraceMode()
{
    if (driver_ == nullptr) {
        return Result::fail(3420, "ZMotionDriver 未初始化");
    }

    return Result::success();
}

// ===================================================================
// 轨迹下发
// ===================================================================

Result TraceProtocol::sendTrajectory(const QString& datFilePath,
                                      int totalPoints,
                                      const TrajectorySendOptions& options,
                                      int eventId)
{
    // 1. 校验系统状态：仅 kSysRobotMode 允许下发
    uint16_t sysState = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, sysState);
    if (!ret.ok) return ret;

    if (sysState != kSysRobotMode) {
        return Result::fail(3415,
            QString("系统状态不允许下发 Trace (当前=%1, 需要=%2)")
                .arg(sysState).arg(kSysRobotMode));
    }

    // 2. 将所有轨迹状态寄存器统一置为 kDataBlank
    for (int i = 0; i < kTrajGroupNum; ++i) {
        ret = driver_->writeModbusReg(kRegTrajStatusBase + i, kDataBlank);
        if (!ret.ok) return ret;
    }

    // 3. 打开文件（流式读取，不在内存中全量加载）
    QFile file(datFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result::fail(3416,
            QString("无法打开轨迹文件：%1，错误：%2")
                .arg(datFilePath, file.errorString()));
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // 4. Trigger
    ret = driver_->trigger();
    if (!ret.ok) {
        file.close();
        return ret;
    }

    int totalGroups = (totalPoints + kTrajGroupSize - 1) / kTrajGroupSize;
    int prefillGroups = qMin(kTrajGroupNum, totalGroups);
    int loopNum = 0;
    float blockData[kTrajBlockSize];

    // ── 内部辅助：从文件读取一组数据到 blockData，写入指定 groupId ──
    auto sendOneGroup = [&](int groupId) -> Result {
        for (int i = 0; i < kTrajGroupSize; ++i) {
            int ptIdx = loopNum * kTrajGroupSize + i;
            float* dest = &blockData[i * kTrajCmdSize];

            if (ptIdx < totalPoints && !in.atEnd()) {
                for (int j = 0; j < kTrajCmdSize; ++j) {
                    in >> dest[j];
                    if (in.status() != QDataStream::Ok) {
                        dest[0] = kCmdMove;
                        for (int k = 1; k < kTrajCmdSize; ++k) dest[k] = 0;
                        in.resetStatus();
                        break;
                    }
                }
            } else {
                dest[0] = kCmdMove;
                for (int k = 1; k < kTrajCmdSize; ++k) dest[k] = 0;
            }
        }

        int tableAddr = kTrajTableStart + groupId * kTrajBlockSize;
        Result r = driver_->setTable(tableAddr, kTrajBlockSize, blockData);
        if (!r.ok) return r;

        r = driver_->writeModbusReg(kRegTrajStatusBase + groupId, kDataUpdate);
        if (!r.ok) return r;

        loopNum++;

        if (options.onProgress) {
            options.onProgress(loopNum, totalGroups);
        }

        return Result::success();
    };

    // 5. 预填充前 prefillGroups 组
    for (; loopNum < prefillGroups; ) {
        if (options.isCancelled && options.isCancelled()) {
            file.close();
            return Result::fail(3405, "轨迹下发已取消");
        }

        if (options.isPaused) {
            while (options.isPaused()) {
                if (options.isCancelled && options.isCancelled()) {
                    file.close();
                    return Result::fail(3405, "轨迹下发已取消");
                }
                QThread::msleep(100);
            }
        }

        int groupId = loopNum % kTrajGroupNum;

        ret = waitBufferReady(groupId, options.isCancelled);
        if (!ret.ok) {
            file.close();
            return ret;
        }

        ret = sendOneGroup(groupId);
        if (!ret.ok) {
            file.close();
            return ret;
        }
    }

    // 6. 预填充完成后，下发事件：开始轨迹运动
    //    Trace 模式为 kEventTraj，轨迹控制模式为 kEventCtrl
    ret = driver_->writeModbusReg(kRegEventLevel2,
                                  static_cast<uint16_t>(eventId));
    if (!ret.ok) {
        file.close();
        return ret;
    }

    // 7. 继续流式补充剩余组
    while (loopNum < totalGroups) {
        if (options.isCancelled && options.isCancelled()) {
            file.close();
            return Result::fail(3405, "轨迹下发已取消");
        }

        if (options.isPaused) {
            while (options.isPaused()) {
                if (options.isCancelled && options.isCancelled()) {
                    file.close();
                    return Result::fail(3405, "轨迹下发已取消");
                }
                QThread::msleep(100);
            }
        }

        int groupId = loopNum % kTrajGroupNum;

        ret = waitBufferReady(groupId, options.isCancelled);
        if (!ret.ok) {
            file.close();
            return ret;
        }

        ret = sendOneGroup(groupId);
        if (!ret.ok) {
            file.close();
            return ret;
        }

        qDebug() << "TraceProtocol sendTrajectory loop:" << loopNum;
    }

    file.close();

    qDebug() << "TraceProtocol sendTrajectory done, total loops:" << loopNum;
    return Result::success();
}

// ===================================================================
// 轨迹下发内部方法，检查数据是否更新
// ===================================================================

Result TraceProtocol::waitBufferReady(int groupId,
                                       const std::function<bool()>& isCancelled)
{
    uint16_t state = 0;
    int addr = kRegTrajStatusBase + groupId;
    int waitedMs = 0;

    while (true) {
        if (isCancelled && isCancelled()) {
            return Result::fail(3406, "轨迹下发已取消");
        }

        Result ret = driver_->readModbusReg(addr, state);
        if (!ret.ok) return ret;

        if (state != kDataUpdate) {
            return Result::success();
        }

        QThread::msleep(1);
        ++waitedMs;

        // 周期性检查系统状态：Error/Estop 时缓冲不会再被消费，立即退出
        if (waitedMs % kTrajWaitStateCheckIntervalMs == 0) {
            uint16_t sysState = 0;
            ret = driver_->readModbusReg(kRegSystemState, sysState);
            if (!ret.ok) return ret;

            if (sysState == kSysError || sysState == kSysEstop) {
                return Result::fail(3417,
                    QString("控制器进入异常状态 (%1)，停止等待轨迹缓冲").arg(sysState));
            }
        }
    }
}
