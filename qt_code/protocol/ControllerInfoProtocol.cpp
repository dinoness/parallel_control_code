#include "ControllerInfoProtocol.h"
#include <QDebug>
#include <QtGlobal>

ControllerInfoProtocol::ControllerInfoProtocol(ZMotionDriver* driver)
    : driver_(driver)
{
}

// ===================================================================
// 系统状态读取
// ===================================================================

Result ControllerInfoProtocol::readSystemState(ControllerStateSnapshot& snapshot)
{
    if (driver_ == nullptr) {
        return Result::fail(3101, "ZMotionDriver 未初始化");
    }

    uint16_t state = 0;
    Result ret = driver_->readModbusReg(kRegSystemState, state);
    if (!ret.ok) return ret;

    snapshot.systemState = state;
    snapshot.systemStateText = systemStateToText(state);
    snapshot.timestamp = QDateTime::currentDateTime();

    return Result::success();
}

QString ControllerInfoProtocol::systemStateToText(uint16_t state)
{
    switch (state) {
    case kSysBoot:       return QString("Boot(0)");
    case kSysBusInit:    return QString("BusInit(1)");
    case kSysServoReady: return QString("ServoReady(2)");
    case kSysHoming:     return QString("Homing(3)");
    case kSysReady:      return QString("Ready(4)");
    case kSysRobotMode:  return QString("RobotMode(5)");
    case kSysRunning:    return QString("Running(6)");
    case kSysPaused:     return QString("Paused(17)");
    case kSysError:      return QString("Error(18)");
    case kSysEstop:      return QString("Estop(19)");
    default:             return QString("Unknown(%1)").arg(state);
    }
}

// ===================================================================
// 传感器 TABLE 批量读取
// ===================================================================

Result ControllerInfoProtocol::readSensorBatch(const SensorTableConfig& config,
                                                int& lastReadIndex,
                                                quint64& lastFrameCounter,
                                                SensorTableBatch& batch)
{
    batch = SensorTableBatch();
    batch.timestamp = QDateTime::currentDateTime();

    if (driver_ == nullptr) {
        return Result::fail(3102, "ZMotionDriver 未初始化");
    }

    // 1. 读取控制器端写索引和帧计数器
    uint16 writeIdxVal = 0;
    uint16 frameCntLow = 0;
    uint16 frameCntHigh = 0;

    Result ret = driver_->readModbusReg(config.writeIndexReg, writeIdxVal);
    if (!ret.ok) return ret;

    // 帧计数器可能为 32 位，暂按两个 16 位寄存器拼接
    // 如果控制器只使用一个寄存器，高位为 0
    ret = driver_->readModbusReg(config.frameCounterReg, frameCntLow);
    if (!ret.ok) return ret;

    int controllerWriteIndex = static_cast<int>(writeIdxVal);
    quint64 currentFrameCounter = static_cast<quint64>(frameCntLow);

    // 2. 计算可读帧数
    int readableFrames = 0;
    if (currentFrameCounter > lastFrameCounter) {
        readableFrames = static_cast<int>(currentFrameCounter - lastFrameCounter);
    } else if (currentFrameCounter < lastFrameCounter) {
        // 控制器帧计数器回绕（几乎不会发生，做保护）
        readableFrames = static_cast<int>(currentFrameCounter);
        lastFrameCounter = 0;
        lastReadIndex = 0;
    }

    batch.availableFrames = readableFrames;

    if (readableFrames == 0) {
        return Result::success();
    }

    // 3. 限制单次读取帧数，防止阻塞急停等操作
    int framesToRead = qMin(readableFrames, config.maxFramesPerRead);
    int droppedFrames = readableFrames - framesToRead;
    if (droppedFrames > 0) {
        batch.overflow = true;
        batch.droppedFrames = droppedFrames;
        qDebug() << "ControllerInfoProtocol: sensor overflow, dropping" << droppedFrames << "frames";
    }

    // 4. 计算环形缓冲读取区间
    int readStartFrame = lastReadIndex;
    int readEndFrame = readStartFrame + framesToRead;

    // 检测环形缓冲覆盖风险：如果可读帧数超过环形容量，说明发生了覆盖
    if (readableFrames > config.ringFrameCapacity) {
        batch.overflow = true;
        batch.droppedFrames = readableFrames - config.ringFrameCapacity;
        // 调整读取起点到最早未覆盖位置
        readStartFrame = controllerWriteIndex - config.ringFrameCapacity + 1;
        if (readStartFrame < 0) readStartFrame = 0;
        framesToRead = qMin(framesToRead, config.ringFrameCapacity);
        readEndFrame = readStartFrame + framesToRead;
    }

    // 5. 分两段处理环形回绕
    int frameFloatCount = config.channelCount;
    int firstSegmentFrames = 0;
    int secondSegmentFrames = 0;

    if (readEndFrame <= config.ringFrameCapacity) {
        // 无回绕，单段读取
        firstSegmentFrames = framesToRead;
        secondSegmentFrames = 0;
    } else {
        // 回绕：先读到环形末尾，再从开头读剩余
        firstSegmentFrames = config.ringFrameCapacity - readStartFrame;
        if (firstSegmentFrames < 0) firstSegmentFrames = 0;
        secondSegmentFrames = framesToRead - firstSegmentFrames;
    }

    quint64 baseCounter = static_cast<quint64>(readStartFrame) > lastFrameCounter
                              ? static_cast<quint64>(readStartFrame)
                              : lastFrameCounter;

    // 6. 读取第一段
    if (firstSegmentFrames > 0) {
        ret = readContiguousFrames(config,
                                   readStartFrame % config.ringFrameCapacity,
                                   firstSegmentFrames,
                                   baseCounter,
                                   batch);
        if (!ret.ok) return ret;
    }

    // 7. 读取第二段（环形回绕部分）
    if (secondSegmentFrames > 0) {
        quint64 segBaseCounter = baseCounter + firstSegmentFrames;
        ret = readContiguousFrames(config,
                                   0,
                                   secondSegmentFrames,
                                   segBaseCounter,
                                   batch);
        if (!ret.ok) return ret;
    }

    // 8. 更新读取位置
    lastReadIndex = readEndFrame % config.ringFrameCapacity;
    lastFrameCounter = baseCounter + framesToRead;

    return Result::success();
}

Result ControllerInfoProtocol::readContiguousFrames(const SensorTableConfig& config,
                                                     int startFrame,
                                                     int frameCount,
                                                     quint64 baseFrameCounter,
                                                     SensorTableBatch& batch)
{
    int frameFloatCount = config.channelCount;
    int tableStart = config.tableBase + startFrame * frameFloatCount;
    int floatCount = frameCount * frameFloatCount;

    QVector<float> rawData;
    Result ret = driver_->getTable(tableStart, floatCount, rawData);
    if (!ret.ok) return ret;

    if (rawData.size() < floatCount) {
        return Result::fail(3103,
            QString("TABLE 返回数据不足: 期望=%1 实际=%2")
                .arg(floatCount).arg(rawData.size()));
    }

    for (int i = 0; i < frameCount; ++i) {
        SensorSampleFrame frame;
        frame.frameCounter = baseFrameCounter + i;
        frame.ringIndex = (startFrame + i) % config.ringFrameCapacity;
        frame.hostTimestamp = QDateTime::currentDateTime();
        frame.values.resize(config.channelCount);

        for (int ch = 0; ch < config.channelCount; ++ch) {
            frame.values[ch] = rawData[i * frameFloatCount + ch];
        }

        batch.frames.append(frame);
    }

    return Result::success();
}

// ===================================================================
// 机器人状态 TABLE 批量读取
// ===================================================================

Result ControllerInfoProtocol::readStatusBatch(quint64& lastFrameCounter,
                                               StatusTableBatch& batch)
{
    batch = StatusTableBatch();
    batch.timestamp = QDateTime::currentDateTime();

    if (driver_ == nullptr) {
        return Result::fail(3601, "ZMotionDriver 未初始化");
    }

    // 1. 读取 header：[frame_counter, write_index]（2 个 float64）
    QVector<float> header;
    Result ret = driver_->getTable(kStatusTableBase, kStatusHeaderFloats, header);
    if (!ret.ok) return ret;

    if (header.size() < kStatusHeaderFloats) {
        return Result::fail(3602,
            QString("状态 header 数据不足: 期望=%1 实际=%2")
                .arg(kStatusHeaderFloats).arg(header.size()));
    }

    // frame_counter 由 float64 转 quint64
    quint64 fc = static_cast<quint64>(static_cast<double>(header[0]));

    // 2. frame_counter=0 表示控制器尚未开始采样；无新帧时返回空 batch
    if (fc == 0 || fc <= lastFrameCounter) {
        return Result::success();
    }

    // 3. 计算本次读取的起止帧号
    quint64 newest = fc;
    quint64 oldestAvailable = (fc > static_cast<quint64>(kStatusRingFrameCapacity))
                                  ? fc - kStatusRingFrameCapacity + 1
                                  : 1;

    quint64 start;
    if (lastFrameCounter == 0) {
        // 首次读取只取最新一帧
        start = newest;
    } else {
        start = qMax(lastFrameCounter + 1, oldestAvailable);
        // 环形区被覆盖，中间帧已丢失
        if (start > lastFrameCounter + 1) {
            batch.overflow = true;
            batch.droppedFrames = start - (lastFrameCounter + 1);
            qDebug() << "ControllerInfoProtocol: status overflow, dropped"
                     << batch.droppedFrames << "frames";
        }
    }

    // 4. start..newest 按环形位置分一到两段连续区间批量读取
    // 帧号 f 的环形位置 = (f-1) % 容量
    int startRing = static_cast<int>((start - 1) % kStatusRingFrameCapacity);
    quint64 totalFrames = newest - start + 1;
    quint64 firstSegFrames = qMin(totalFrames,
                                  static_cast<quint64>(kStatusRingFrameCapacity - startRing));

    for (int seg = 0; seg < 2; ++seg) {
        quint64 segFrames = (seg == 0) ? firstSegFrames
                                       : totalFrames - firstSegFrames;
        if (segFrames == 0) break;

        int segRingStart = (seg == 0) ? startRing : 0;
        quint64 segFrameBase = (seg == 0) ? start : start + firstSegFrames;
        int floatCount = static_cast<int>(segFrames) * kStatusChannelCount;

        QVector<float> raw;
        ret = driver_->getTable(kStatusFrameBase + segRingStart * kStatusChannelCount,
                                floatCount, raw);
        if (!ret.ok) return ret;

        if (raw.size() < floatCount) {
            return Result::fail(3603,
                QString("状态帧数据不足: 期望=%1 实际=%2")
                    .arg(floatCount).arg(raw.size()));
        }

        // 5. 逐帧解析填充 StatusSampleFrame
        for (int i = 0; i < static_cast<int>(segFrames); ++i) {
            const float* ch = raw.constData() + i * kStatusChannelCount;

            StatusSampleFrame frame;
            frame.frameCounter = segFrameBase + i;
            frame.hostTimestamp = QDateTime::currentDateTime();

            for (int j = 0; j < 5; ++j) {
                frame.dL[j]      = ch[j];       // 支链伸缩量 um
                frame.encoder[j] = ch[5 + j];   // 电机编码器 脉冲
                frame.torque[j]  = ch[10 + j];  // 电机扭矩 千分比
                frame.ee[j]      = ch[15 + j];  // 末端位姿 x,y,z(um), phi,theta(角秒)
            }
            frame.eeValid = (ch[20] > 0.5f);

            batch.frames.append(frame);
        }
    }

    // 6. 更新最后读取帧号
    lastFrameCounter = newest;

    return Result::success();
}
