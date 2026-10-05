#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QString>

MainWindow::MainWindow(AppContext* context, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ctx_(context)
{
    ui->setupUi(this);
    setWindowTitle("M");

    Init();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::Init()
{
    if (ctx_ == nullptr) {
        QMessageBox::critical(this, "错误", "AppContext 未初始化");
        return;
    }

    // ── 初始按钮状态 ──────────────────────────────────
    ui->btn_direct_joint_send->setEnabled(false);
    ui->btn_direct_joint_exit->setEnabled(false);
    ui->btn_jog_send->setEnabled(false);
    ui->btn_jog_exit->setEnabled(false);
    ui->btn_thread_open->setEnabled(false);
    ui->btn_thread_close->setEnabled(false);
    ui->btn_trace_send->setEnabled(false);
    ui->btn_trace_exit->setEnabled(false);
    ui->btn_ctrl_exit->setEnabled(false);
    ui->btn_ctrl_send->setEnabled(false);
    ui->btn_robo_mode_in->setEnabled(true);
    ui->btn_robo_mode_out->setEnabled(false);

    // ── 输入框默认值 ──────────────────────────────────
    ui->ledit_d_j1->setText("0");
    ui->ledit_d_j2->setText("0");
    ui->ledit_d_j3->setText("0");
    ui->ledit_d_j4->setText("0");
    ui->ledit_d_j5->setText("0");
    ui->ledit_jog_x->setText("0");
    ui->ledit_jog_y->setText("0");
    ui->ledit_jog_z->setText("0");
    ui->ledit_jog_phi->setText("0");
    ui->ledit_jog_theta->setText("0");

    // 点动指令下拉框：显示指令名称，userData 为指令代号
    ui->cbox_cmd_id->clear();
    ui->cbox_cmd_id->addItem("MOVE", kCmdMove);
    ui->cbox_cmd_id->addItem("MOVEABS", kCmdMoveAbs);
    ui->cbox_cmd_id->setCurrentIndex(0);  // 默认 MOVE

    // 速度输入框：单位 mm/s，范围 (0, kMaxManualSpeedMmPs]
    ui->ledit_speed->setText("5");
    ui->ledit_speed->setValidator(
        new QDoubleValidator(0.01, kMaxManualSpeedMmPs, 2, this));

    updateMotionModeDisplay();

    // ── 连接状态 ──────────────────────────────────────
    connect(ctx_->connectionService(), &ConnectionService::connectionChanged,
            this, [this](bool connected) {
                ui->statusbar->showMessage(connected ? "已连接" : "未连接");

                if (connected) {
                    ui->label_system_state->setText("Reading...");
                    Result ret = ctx_->controllerInfoService()->startStateMonitor(kControllerStatePollIntervalMs);
                    if (!ret.ok) {
                        qDebug() << "Start state monitor failed:" << ret.message;
                    }
                    ret = ctx_->controllerInfoService()->startStatusMonitor();
                    if (!ret.ok) {
                        qDebug() << "Start status monitor failed:" << ret.message;
                    }
                } else {
                    ctx_->controllerInfoService()->stopAll();
                    ui->label_system_state->setText("Disconnected");
                    robotModeActive_ = false;
                    updateRobotModeButtons();

                    // 断线时复位运动模式相关 UI 状态，
                    // 避免在模式中断线后重连时 ENTER 全部锁死、无法退出
                    resetMotionModeUi();
                    if (paused_) {
                        paused_ = false;
                        ui->btn_pause_resume->setText("PAUSE");
                    }

                    // 断线时复位机器人状态显示 label
                    QLabel* statusLabels[20] = {
                        ui->label_dL_J1, ui->label_dL_J2, ui->label_dL_J3,
                        ui->label_dL_J4, ui->label_dL_J5,
                        ui->label_motor_encoder1, ui->label_motor_encoder2,
                        ui->label_motor_encoder3, ui->label_motor_encoder4,
                        ui->label_motor_encoder5,
                        ui->label_motor_torque1, ui->label_motor_torque2,
                        ui->label_motor_torque3, ui->label_motor_torque4,
                        ui->label_motor_torque5,
                        ui->label_ee_x, ui->label_ee_y, ui->label_ee_z,
                        ui->label_ee_phi, ui->label_ee_theta
                    };
                    for (int i = 0; i < 20; ++i) {
                        statusLabels[i]->setText("-");
                    }
                }
            });

    // ── 控制器状态监控 ────────────────────────────────
    connect(ctx_->controllerInfoService(), &ControllerInfoService::stateUpdated,
            this, [this](const ControllerStateSnapshot& snapshot) {
                ui->label_system_state->setText(snapshot.systemStateText);

                // 以控制器实际状态校准 Robot Mode 标志，
                // 避免控制器端未进入/异常退出 Robot Mode 时 UI 按钮状态锁死
                bool active = (snapshot.systemState == kSysRobotMode
                            || snapshot.systemState == kSysRunning
                            || snapshot.systemState == kSysPaused);
                if (active != robotModeActive_) {
                    robotModeActive_ = active;
                    updateRobotModeButtons();
                }
            });

    connect(ctx_->controllerInfoService(), &ControllerInfoService::monitorError,
            this, [this](const Result& result) {
                qDebug() << "Controller info monitor error:" << result.message;
            });

    // ── 传感器数据预留 ────────────────────────────────
    connect(ctx_->controllerInfoService(), &ControllerInfoService::sensorBatchReceived,
            this, [this](const SensorTableBatch& batch) {
                qDebug() << "Sensor batch frames:" << batch.frames.size()
                         << "overflow:" << batch.overflow
                         << "dropped:" << batch.droppedFrames;
            });

    // ── 机器人状态数据显示 ────────────────────────────
    connect(ctx_->controllerInfoService(), &ControllerInfoService::statusBatchReceived,
            this, [this](const StatusTableBatch& batch) {
                // 只显示最新一帧
                const StatusSampleFrame& frame = batch.frames.last();

                QLabel* dLLabels[5] = { ui->label_dL_J1, ui->label_dL_J2, ui->label_dL_J3,
                                        ui->label_dL_J4, ui->label_dL_J5 };
                QLabel* encoderLabels[5] = { ui->label_motor_encoder1, ui->label_motor_encoder2,
                                             ui->label_motor_encoder3, ui->label_motor_encoder4,
                                             ui->label_motor_encoder5 };
                QLabel* torqueLabels[5] = { ui->label_motor_torque1, ui->label_motor_torque2,
                                            ui->label_motor_torque3, ui->label_motor_torque4,
                                            ui->label_motor_torque5 };
                QLabel* eeLabels[3] = { ui->label_ee_x, ui->label_ee_y, ui->label_ee_z };

                for (int i = 0; i < 5; ++i) {
                    dLLabels[i]->setText(QString::number(frame.dL[i], 'f', 2));
                    encoderLabels[i]->setText(QString::number(frame.encoder[i], 'f', 0));
                    torqueLabels[i]->setText(QString::number(frame.torque[i], 'f', 0));
                }

                if (frame.eeValid) {
                    for (int i = 0; i < 3; ++i) {
                        eeLabels[i]->setText(QString::number(frame.ee[i], 'f', 2));
                    }
                    // phi/theta 单位为角秒，UI 显示度需 ÷3600
                    ui->label_ee_phi->setText(QString::number(frame.ee[3] / 3600.0, 'f', 4));
                    ui->label_ee_theta->setText(QString::number(frame.ee[4] / 3600.0, 'f', 4));
                } else {
                    for (int i = 0; i < 3; ++i) {
                        eeLabels[i]->setText("-");
                    }
                    ui->label_ee_phi->setText("-");
                    ui->label_ee_theta->setText("-");
                }
            });

    // ── 轨迹下发进度 ──────────────────────────────────
    connect(ctx_->trajectoryService(), &TrajectoryService::sendProgressChanged,
            this, [this](int sentGroups, int totalGroups, int percent) {
                ui->statusbar->showMessage(
                    QString("轨迹下发中... %1% (%2/%3 组)")
                        .arg(percent).arg(sentGroups).arg(totalGroups));
            });

    // ── 轨迹下发完成 ──────────────────────────────────
    connect(ctx_->trajectoryService(), &TrajectoryService::sendFinished,
            this, [this](const Result& result) {
                if (result.ok) {
                    ui->statusbar->showMessage("轨迹下发完成", 5000);
                    QMessageBox::information(this, "成功", "轨迹下发完成");
                } else {
                    ui->statusbar->showMessage("轨迹下发失败", 5000);
                    QMessageBox::warning(this, "轨迹下发失败", result.message);
                }
            });

    // ── 下发状态切换 ──────────────────────────────────
    connect(ctx_->trajectoryService(), &TrajectoryService::sendingStateChanged,
            this, [this](bool sending) {
                if (sending) {
                    // 发送中: Thread Open 充当 Cancel
                    ui->btn_thread_open->setEnabled(true);
                    ui->btn_trace_send->setEnabled(false);
                    ui->btn_thread_close->setEnabled(false);
                } else {
                    // 发送结束: 恢复 Trace 模式按钮状态
                    updateTraceButtonStates();
                }
                updateCtrlButtonStates();
            });
}

// ===================================================================
// 模式管理
// ===================================================================

bool MainWindow::canEnterMode(MotionMode mode)
{
    if (currentMotionMode_ == mode) {
        QMessageBox::information(this, "提示", "已在当前模式中");
        return false;
    }
    if (currentMotionMode_ != MotionMode::None) {
        QMessageBox::warning(this, "提示", "请先退出当前运动模式");
        return false;
    }
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return false;
    }
    // Cart Jog / Trace / Ctrl Trace 必须先进入 Robot Mode
    if ((mode == MotionMode::CartJog || mode == MotionMode::Trace
         || mode == MotionMode::CtrlTrace)
        && !robotModeActive_) {
        QMessageBox::warning(this, "错误", "请先进入 Robot Mode");
        return false;
    }
    return true;
}

void MainWindow::enterMode(MotionMode mode)
{
    // ── 切换模式状态（不在此处下发事件）──────────────────
    if (mode == MotionMode::DirectJoint) {
        Result ret = ctx_->motionService()->enterJointMode();
        if (!ret.ok) {
            QMessageBox::warning(this, "错误", "进入 Joint 模式失败: " + ret.message);
            return;
        }
    } else if (mode == MotionMode::CartJog) {
        Result ret = ctx_->motionService()->enterCartJogMode();
        if (!ret.ok) {
            QMessageBox::warning(this, "错误", "进入 Cart Jog 模式失败: " + ret.message);
            return;
        }
    } else if (mode == MotionMode::Trace) {
        Result ret = ctx_->motionService()->enterTraceMode();
        if (!ret.ok) {
            QMessageBox::warning(this, "错误", "进入 Trace 模式失败: " + ret.message);
            return;
        }
    } else if (mode == MotionMode::CtrlTrace) {
        // Ctrl Trace 与 Trace 共用协议链路，进入校验相同
        Result ret = ctx_->motionService()->enterTraceMode();
        if (!ret.ok) {
            QMessageBox::warning(this, "错误", "进入 Ctrl Trace 模式失败: " + ret.message);
            return;
        }
    }

    currentMotionMode_ = mode;
    updateMotionModeDisplay();

    // 禁用所有 ENTER，启用对应模式的 EXIT
    ui->btn_direct_joint_enter->setEnabled(false);
    ui->btn_direct_joint_exit->setEnabled(mode == MotionMode::DirectJoint);
    ui->btn_direct_joint_send->setEnabled(mode == MotionMode::DirectJoint);

    ui->btn_jog_enter->setEnabled(false);
    ui->btn_jog_exit->setEnabled(mode == MotionMode::CartJog);
    ui->btn_jog_send->setEnabled(mode == MotionMode::CartJog);

    ui->btn_trace_enter->setEnabled(false);
    ui->btn_trace_exit->setEnabled(mode == MotionMode::Trace);

    ui->btn_ctrl_enter->setEnabled(false);
    ui->btn_ctrl_exit->setEnabled(mode == MotionMode::CtrlTrace);

    if (mode == MotionMode::Trace || mode == MotionMode::CtrlTrace) {
        // SEND 需先 Thread Open，由按钮状态函数统一控制
        updateTraceButtonStates();
        updateCtrlButtonStates();
    }
}

void MainWindow::exitCurrentMode()
{
    // Trace / Ctrl Trace 共用下发线程，退出前统一关闭（内部会停止下发）
    if ((currentMotionMode_ == MotionMode::Trace
         || currentMotionMode_ == MotionMode::CtrlTrace)
        && (traceThreadOpened_ || ctx_->trajectoryService()->isSending())) {
        closeTraceThread();
    }

    // ── 切换模式状态，不在此处下发事件 ──────────────────
    if (currentMotionMode_ == MotionMode::DirectJoint) {
        ctx_->motionService()->exitJointMode();
    } else if (currentMotionMode_ == MotionMode::CartJog) {
        ctx_->motionService()->exitCartJogMode();
    } else if (currentMotionMode_ == MotionMode::Trace
               || currentMotionMode_ == MotionMode::CtrlTrace) {
        // Ctrl Trace 与 Trace 共用协议链路，退出处理相同
        ctx_->motionService()->exitTraceMode();
    }

    resetMotionModeUi();
}

void MainWindow::resetMotionModeUi()
{
    currentMotionMode_ = MotionMode::None;
    traceThreadOpened_ = false;
    updateMotionModeDisplay();

    // 恢复 ENTER，禁用所有 EXIT/SEND
    ui->btn_direct_joint_enter->setEnabled(true);
    ui->btn_direct_joint_exit->setEnabled(false);
    ui->btn_direct_joint_send->setEnabled(false);

    ui->btn_jog_enter->setEnabled(true);
    ui->btn_jog_exit->setEnabled(false);
    ui->btn_jog_send->setEnabled(false);

    ui->btn_trace_enter->setEnabled(true);
    ui->btn_trace_exit->setEnabled(false);

    ui->btn_ctrl_enter->setEnabled(true);
    ui->btn_ctrl_exit->setEnabled(false);
    ui->btn_ctrl_send->setEnabled(false);

    ui->btn_thread_open->setEnabled(false);
    ui->btn_thread_close->setEnabled(false);
    ui->btn_trace_send->setEnabled(false);
}

void MainWindow::updateMotionModeDisplay()
{
    switch (currentMotionMode_) {
    case MotionMode::None:        ui->label_motion_mode->setText("None");        break;
    case MotionMode::DirectJoint: ui->label_motion_mode->setText("Direct Joint"); break;
    case MotionMode::CartJog:     ui->label_motion_mode->setText("Cart Jog");     break;
    case MotionMode::Trace:       ui->label_motion_mode->setText("Trace");       break;
    case MotionMode::CtrlTrace:   ui->label_motion_mode->setText("Ctrl Trace");  break;
    }
}

void MainWindow::updateTraceButtonStates()
{
    // Thread Open/Close 为 Trace 与 Ctrl Trace 共用
    bool inTraceMode = (currentMotionMode_ == MotionMode::Trace
                        || currentMotionMode_ == MotionMode::CtrlTrace);
    bool sending = ctx_->trajectoryService()->isSending();

    if (!inTraceMode || sending) {
        ui->btn_thread_open->setEnabled(sending);
        ui->btn_thread_close->setEnabled(false);
        ui->btn_trace_send->setEnabled(false);
        return;
    }

    // Trace / Ctrl Trace 模式，非发送状态
    ui->btn_thread_open->setEnabled(!traceThreadOpened_);
    ui->btn_thread_close->setEnabled(traceThreadOpened_);
    ui->btn_trace_send->setEnabled(traceThreadOpened_
                                   && currentMotionMode_ == MotionMode::Trace);
}

void MainWindow::updateCtrlButtonStates()
{
    bool inCtrl = (currentMotionMode_ == MotionMode::CtrlTrace);
    bool sending = ctx_->trajectoryService()->isSending();

    // Ctrl Trace 与 Trace 一样需先 Thread Open，SEND 才可点
    ui->btn_ctrl_send->setEnabled(inCtrl && !sending && traceThreadOpened_);
}

void MainWindow::enterRobotMode()
{
    if (robotModeActive_) {
        QMessageBox::information(this, "提示", "已在 Robot Mode 中");
        return;
    }

    Result ret = ctx_->motionService()->enterRobotMode();
    if (!ret.ok) {
        QMessageBox::warning(this, "进入 Robot Mode 失败", ret.message);
        return;
    }

    robotModeActive_ = true;
    updateRobotModeButtons();
    ui->statusbar->showMessage("进入 Robot Mode", 3000);
    qDebug() << "RobotModeIn command sent to MODBUS[" << kRegEventLevel2
             << "] = " << kEventRobotIn;
}

void MainWindow::exitRobotMode()
{
    if (!robotModeActive_) return;

    Result ret = ctx_->motionService()->exitRobotMode();
    if (!ret.ok) {
        QMessageBox::warning(this, "退出 Robot Mode 失败", ret.message);
        return;
    }

    robotModeActive_ = false;
    updateRobotModeButtons();
    ui->statusbar->showMessage("退出 Robot Mode", 3000);
    qDebug() << "RobotModeOut command sent to MODBUS[" << kRegEventLevel2
             << "] = " << kEventRobotOut;
}

void MainWindow::updateRobotModeButtons()
{
    ui->btn_robo_mode_in->setEnabled(!robotModeActive_);
    ui->btn_robo_mode_out->setEnabled(robotModeActive_);
}



// ===================================================================
// Home
// ===================================================================

void MainWindow::sendHomeCmd()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    Result ret = ctx_->motionService()->sendHome();
    if (!ret.ok) {
        QMessageBox::warning(this, "回零失败", ret.message);
        ui->statusbar->showMessage("回零指令下发失败", 3000);
        return;
    }

    ui->statusbar->showMessage("回零指令已下发", 3000);
    qDebug() << "Home command sent to MODBUS[" << kRegEventLevel2
             << "] = " << kEventHome;
}

// ===================================================================
// Direct Joint
// ===================================================================

void MainWindow::sendDirectJointCmd()
{
    float j1 = ui->ledit_d_j1->text().toFloat();
    float j2 = ui->ledit_d_j2->text().toFloat();
    float j3 = ui->ledit_d_j3->text().toFloat();
    float j4 = ui->ledit_d_j4->text().toFloat();
    float j5 = ui->ledit_d_j5->text().toFloat();
    float speed = ui->ledit_speed->text().toFloat();  // mm/s
    if (speed <= 0.0f || speed > kMaxManualSpeedMmPs) {
        QMessageBox::warning(this, "参数错误",
                             QString("速度需在 (0, %1] mm/s 范围内").arg(kMaxManualSpeedMmPs));
        return;
    }

    Result ret = ctx_->motionService()->sendDirectJoint(j1, j2, j3, j4, j5, speed);
    if (!ret.ok) {
        QMessageBox::warning(this, "下发失败", ret.message);
        return;
    }

    ui->statusbar->showMessage(
        QString("Direct Joint: J1=%1 J2=%2 J3=%3 J4=%4 J5=%5 Speed=%6mm/s")
            .arg(j1).arg(j2).arg(j3).arg(j4).arg(j5).arg(speed), 3000);
    qDebug() << "Direct Joint command sent.";
}

// ===================================================================
// Cart Jog
// ===================================================================

void MainWindow::sendCartJogCmd()
{
    int cmdId = ui->cbox_cmd_id->currentData().toInt();
    float x = ui->ledit_jog_x->text().toFloat();
    float y = ui->ledit_jog_y->text().toFloat();
    float z = ui->ledit_jog_z->text().toFloat();
    float phi = ui->ledit_jog_phi->text().toFloat();
    float theta = ui->ledit_jog_theta->text().toFloat();
    float speed = ui->ledit_speed->text().toFloat();  // mm/s
    if (speed <= 0.0f || speed > kMaxManualSpeedMmPs) {
        QMessageBox::warning(this, "参数错误",
                             QString("速度需在 (0, %1] mm/s 范围内").arg(kMaxManualSpeedMmPs));
        return;
    }

    Result ret = ctx_->motionService()->sendCartJog(cmdId, x, y, z, phi, theta, speed);
    if (!ret.ok) {
        QMessageBox::warning(this, "下发失败", ret.message);
        return;
    }

    ui->statusbar->showMessage(
        QString("Cart Jog: Cmd=%1 X=%2 Y=%3 Z=%4 Phi=%5 Theta=%6 Speed=%7mm/s")
            .arg(ui->cbox_cmd_id->currentText()).arg(x).arg(y).arg(z).arg(phi).arg(theta).arg(speed), 3000);
    qDebug() << "Cart Jog command sent.";
}

// ===================================================================
// Trace
// ===================================================================

void MainWindow::openTraceThread()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    // TODO: 调用 MotionService / TrajectoryService 的线程准备接口
    // 当前 TrajectoryService::startSendTrajectoryAsync() 内部自行管理线程生命周期，
    // Thread Open 作为预备状态，使能 SEND 按钮
    traceThreadOpened_ = true;
    updateTraceButtonStates();
    updateCtrlButtonStates();  // Ctrl Trace 模式：Thread Open 后同步使能 SEND
    ui->statusbar->showMessage("Trace 线程已开启");
    qDebug() << "Trace thread opened.";
}

void MainWindow::closeTraceThread()
{
    // 如果正在下发，先取消并等待线程结束
    if (ctx_->trajectoryService()->isSending()) {
        ctx_->trajectoryService()->stopSendThread(5000);
    }

    traceThreadOpened_ = false;
    updateTraceButtonStates();
    updateCtrlButtonStates();  // Ctrl Trace 模式：Thread Close 后同步禁用 SEND
    ui->statusbar->showMessage("Trace 线程已关闭");
    qDebug() << "Trace thread closed.";
}

void MainWindow::sendTraceFile()
{
    if (ctx_->trajectoryService()->isSending()) {
        QMessageBox::warning(this, "提示", "当前已有轨迹正在下发");
        return;
    }

    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    QString fileName = ui->ledit_data_file_name->text();
    if (fileName.isEmpty()) {
        fileName = "01";
    }

    Result ret = ctx_->trajectoryService()->startSendTrajectoryAsync(fileName);

    if (!ret.ok) {
        QMessageBox::warning(this, "轨迹下发启动失败", ret.message);
        return;
    }

    ui->statusbar->showMessage("轨迹下发已启动...");
}

// ===================================================================
// Ctrl Trace（轨迹控制）
// ===================================================================

void MainWindow::sendCtrlTraceFile()
{
    if (ctx_->trajectoryService()->isSending()) {
        QMessageBox::warning(this, "提示", "当前已有轨迹正在下发");
        return;
    }

    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    QString fileName = ui->ledit_data_file_name->text();
    if (fileName.isEmpty()) {
        fileName = "01";
    }

    // 下发流程与 Trace 完全相同，仅启动事件替换为 kEventCtrl
    Result ret = ctx_->trajectoryService()->startSendTrajectoryAsync(fileName, kEventCtrl);

    if (!ret.ok) {
        QMessageBox::warning(this, "轨迹控制下发启动失败", ret.message);
        return;
    }

    ui->statusbar->showMessage("轨迹控制下发已启动...");
}

// ===================================================================
// Connection
// ===================================================================

void MainWindow::ip_Scan()
{
    if (ctx_ == nullptr) return;

    ui->comboBox->clear();
    ui->comboBox->addItem("127.0.0.1");

    QStringList ipList;
    Result ret = ctx_->connectionService()->ipScan(ipList);

    if (!ret.ok) {
        qDebug() << "IP scan failed:" << ret.message;
        return;
    }

    ui->comboBox->addItems(ipList);
    qDebug() << "IP scan found:" << ipList;
}


void MainWindow::connect_Ether()
{
    if (ctx_ == nullptr) {
        QMessageBox::warning(this, "错误", "AppContext 为空");
        return;
    }

    QString ip = ui->comboBox->currentText();

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入控制器 IP");
        return;
    }

    Result ret = ctx_->connectionService()->connectToController(ip);

    if (!ret.ok) {
        QMessageBox::warning(this, "连接失败", ret.message);
        return;
    }

    QMessageBox::information(this, "连接成功", "控制器连接成功");
}



void MainWindow::closeEther()
{
    if (ctx_ == nullptr) return;

    // 在断开连接前先停止监控线程，避免 Worker 继续读取已关闭的 handle
    ctx_->controllerInfoService()->stopAll();

    Result ret = ctx_->connectionService()->disconnectFromController();

    if (!ret.ok) {
        QMessageBox::warning(this, "断开失败", ret.message);
        return;
    }

    QMessageBox::information(this, "已断开", "控制器连接已断开");
}

// ===================================================================
// Data
// ===================================================================

void MainWindow::dat_to_xlsx()
{
    if (ctx_ == nullptr) return;

    QString datFile = ui->ledit_data_file_name->text();
    QString csvFile = ui->ledit_csv_file_name->text();
    if (datFile.isEmpty()) datFile = "01";
    if (csvFile.isEmpty()) csvFile = datFile;

    Result ret = ctx_->trajectoryService()->datToCsv(datFile, csvFile);

    if (!ret.ok) {
        QMessageBox::warning(this, "导出失败", ret.message);
        return;
    }

    qDebug() << "dat_to_csv over";
}

void MainWindow::xlsx_to_dat()
{
    if (ctx_ == nullptr) return;

    QString csvFile = ui->ledit_csv_file_name->text();
    QString datFile = ui->ledit_data_file_name->text();
    if (csvFile.isEmpty()) csvFile = "01";
    if (datFile.isEmpty()) datFile = csvFile;

    Result ret = ctx_->trajectoryService()->csvToDat(csvFile, datFile);

    if (!ret.ok) {
        QMessageBox::warning(this, "导入失败", ret.message);
        return;
    }

    qDebug() << "csv_to_dat over";
}

void MainWindow::motion_cmd()
{
    if (ctx_ == nullptr) return;

    if (ctx_->trajectoryService()->isSending()) {
        QMessageBox::warning(this, "提示", "当前已有轨迹正在下发");
        return;
    }

    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    QString fileName = ui->ledit_data_file_name->text();
    if (fileName.isEmpty()) {
        fileName = "01";
    }

    Result ret = ctx_->trajectoryService()->startSendTrajectoryAsync(fileName);

    if (!ret.ok) {
        QMessageBox::warning(this, "轨迹下发启动失败", ret.message);
        return;
    }
}

// ===================================================================
// SLOT FUNCTIONS
// ===================================================================

// ── Connection ────────────────────────────────────────

void MainWindow::on_btn_ip_scan_clicked()
{
    ip_Scan();
}

void MainWindow::on_btn_connect_controller_clicked()
{
    connect_Ether();
}


void MainWindow::on_btn_disconnect_controller_clicked()
{
    closeEther();
}

// ── Home ──────────────────────────────────────────────

void MainWindow::on_btn_home_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }
    sendHomeCmd();
}

// ── Direct Joint ──────────────────────────────────────

void MainWindow::on_btn_direct_joint_enter_clicked()
{
    // 进入 Direct Joint 前自动退出 Robot Mode
    if (robotModeActive_) {
        exitRobotMode();
    }
    if (!canEnterMode(MotionMode::DirectJoint)) return;
    enterMode(MotionMode::DirectJoint);
    ui->statusbar->showMessage("进入 Direct Joint 模式");
}


void MainWindow::on_btn_direct_joint_exit_clicked()
{
    exitCurrentMode();
    ui->statusbar->showMessage("退出 Direct Joint 模式");
}


void MainWindow::on_btn_direct_joint_send_clicked()
{
    if (currentMotionMode_ != MotionMode::DirectJoint) {
        QMessageBox::warning(this, "错误", "请先进入 Direct Joint 模式");
        return;
    }
    sendDirectJointCmd();
}

// ── Cart Jog ──────────────────────────────────────────

void MainWindow::on_btn_jog_enter_clicked()
{
    if (!canEnterMode(MotionMode::CartJog)) return;
    enterMode(MotionMode::CartJog);
    ui->statusbar->showMessage("进入 Cart Jog 模式");
}


void MainWindow::on_btn_jog_exit_clicked()
{
    exitCurrentMode();
    ui->statusbar->showMessage("退出 Cart Jog 模式");
}


void MainWindow::on_btn_jog_send_clicked()
{
    if (currentMotionMode_ != MotionMode::CartJog) {
        QMessageBox::warning(this, "错误", "请先进入 Cart Jog 模式");
        return;
    }
    sendCartJogCmd();
}

// ── Trace ─────────────────────────────────────────────

void MainWindow::on_btn_trace_enter_clicked()
{
    if (!canEnterMode(MotionMode::Trace)) return;
    enterMode(MotionMode::Trace);
    ui->statusbar->showMessage("进入 Trace 模式，请点击 Thread Open 开启线程");
}


void MainWindow::on_btn_trace_exit_clicked()
{
    if (currentMotionMode_ != MotionMode::Trace) {
        QMessageBox::warning(this, "错误", "当前不在 Trace 模式");
        return;
    }

    // 检查控制器是否正在运行轨迹
    Result ret = ctx_->motionService()->canExitTraceMode();
    if (!ret.ok) {
        QMessageBox::warning(this, "无法退出", ret.message);
        return;
    }

    exitCurrentMode();
    ui->statusbar->showMessage("退出 Trace 模式");
}


void MainWindow::on_btn_thread_open_clicked()
{
    // Trace / Ctrl Trace 模式 → 开启线程（两模式共用下发线程）
    if (currentMotionMode_ != MotionMode::Trace
        && currentMotionMode_ != MotionMode::CtrlTrace) {
        QMessageBox::warning(this, "错误", "请先进入 Trace 或 Ctrl Trace 模式");
        return;
    }

    // 发送中：Thread Open 充当 Cancel（与 updateTraceButtonStates 的使能逻辑对应）
    if (ctx_->trajectoryService()->isSending()) {
        ctx_->trajectoryService()->cancelSendTrajectory();
        ui->statusbar->showMessage("轨迹下发已请求取消");
        qDebug() << "Trajectory send cancel requested.";
        return;
    }

    if (traceThreadOpened_) {
        QMessageBox::information(this, "提示", "线程已开启");
        return;
    }

    openTraceThread();
}


void MainWindow::on_btn_thread_close_clicked()
{
    if (currentMotionMode_ != MotionMode::Trace
        && currentMotionMode_ != MotionMode::CtrlTrace) return;

    if (!traceThreadOpened_ && !ctx_->trajectoryService()->isSending()) {
        QMessageBox::information(this, "提示", "线程未开启");
        return;
    }

    closeTraceThread();
}


void MainWindow::on_btn_trace_send_clicked()
{
    if (currentMotionMode_ != MotionMode::Trace) {
        QMessageBox::warning(this, "错误", "请先进入 Trace 模式");
        return;
    }

    if (!traceThreadOpened_) {
        QMessageBox::warning(this, "错误", "请先点击 Thread Open 开启线程");
        return;
    }

    sendTraceFile();
}

// ── Ctrl Trace ────────────────────────────────────────

void MainWindow::on_btn_ctrl_enter_clicked()
{
    if (!canEnterMode(MotionMode::CtrlTrace)) return;
    enterMode(MotionMode::CtrlTrace);
    ui->statusbar->showMessage("进入 Ctrl Trace 模式");
}


void MainWindow::on_btn_ctrl_exit_clicked()
{
    if (currentMotionMode_ != MotionMode::CtrlTrace) {
        QMessageBox::warning(this, "错误", "当前不在 Ctrl Trace 模式");
        return;
    }

    // 与 Trace 相同：控制器运行中不允许退出
    Result ret = ctx_->motionService()->canExitTraceMode();
    if (!ret.ok) {
        QMessageBox::warning(this, "无法退出", ret.message);
        return;
    }

    exitCurrentMode();
    ui->statusbar->showMessage("退出 Ctrl Trace 模式");
}


void MainWindow::on_btn_ctrl_send_clicked()
{
    if (currentMotionMode_ != MotionMode::CtrlTrace) {
        QMessageBox::warning(this, "错误", "请先进入 Ctrl Trace 模式");
        return;
    }

    if (!traceThreadOpened_) {
        QMessageBox::warning(this, "错误", "请先点击 Thread Open 开启线程");
        return;
    }

    sendCtrlTraceFile();
}

// ── Data ──────────────────────────────────────────────

void MainWindow::on_btn_trace_test_clicked()
{
    motion_cmd();
}



void MainWindow::on_btn_trace_to_xlsx_clicked()
{
    dat_to_xlsx();
}


void MainWindow::on_btn_xlsx_to_dat_clicked()
{
    xlsx_to_dat();
}


void MainWindow::on_btn_pause_resume_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    if (paused_) {
        // ── RESUME ──────────────────────────────────
        Result ret = ctx_->motionService()->sendResume();
        if (!ret.ok) {
            QMessageBox::warning(this, "Resume 失败", ret.message);
            return;
        }

        // Trace / Ctrl Trace 模式：恢复轨迹下发
        if (currentMotionMode_ == MotionMode::Trace
            || currentMotionMode_ == MotionMode::CtrlTrace) {
            ctx_->trajectoryService()->resumeSendTrajectory();
        }

        paused_ = false;
        ui->btn_pause_resume->setText("PAUSE");
        ui->statusbar->showMessage("Resume 指令已下发", 3000);
        qDebug() << "Resume command sent to MODBUS[" << kRegEventLevel2
                 << "] = " << kEventResume;
    } else {
        // ── PAUSE ────────────────────────────────────
        Result ret = ctx_->motionService()->sendPause();
        if (!ret.ok) {
            QMessageBox::warning(this, "Pause 失败", ret.message);
            return;
        }

        // Trace / Ctrl Trace 模式：暂停轨迹下发，保留进度
        if (currentMotionMode_ == MotionMode::Trace
            || currentMotionMode_ == MotionMode::CtrlTrace) {
            ctx_->trajectoryService()->pauseSendTrajectory();
        }

        paused_ = true;
        ui->btn_pause_resume->setText("RESUME");
        ui->statusbar->showMessage("Pause 指令已下发", 3000);
        qDebug() << "Pause command sent to MODBUS[" << kRegEventLevel2
                 << "] = " << kEventPause;
    }
}


void MainWindow::on_btn_stop_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    Result ret = ctx_->motionService()->sendStop();
    if (!ret.ok) {
        QMessageBox::warning(this, "Stop 失败", ret.message);
        return;
    }

    // Trace / Ctrl Trace 模式：结束当前轨迹下发，不保留进度
    if (currentMotionMode_ == MotionMode::Trace
        || currentMotionMode_ == MotionMode::CtrlTrace) {
        ctx_->trajectoryService()->stopSendThread(5000);
    }

    // 恢复 PAUSE 按钮状态
    if (paused_) {
        paused_ = false;
        ui->btn_pause_resume->setText("PAUSE");
    }

    ui->statusbar->showMessage("Stop 指令已下发", 3000);
    qDebug() << "Stop command sent to MODBUS[" << kRegEventLevel2
             << "] = " << kEventStop;
}


void MainWindow::on_btn_emergency_stop_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    // 如果正在轨迹下发，先请求 worker 尽快退出（不等待线程结束，避免阻塞 UI）
    ctx_->trajectoryService()->cancelSendTrajectory();

    // 执行软件急停：RapidStop(2) + ESTOP 事件
    Result ret = ctx_->motionService()->sendEstop();
    if (!ret.ok) {
        QMessageBox::warning(this, "Emergency Stop 失败", ret.message);
        return;
    }

    // 恢复 PAUSE 按钮状态
    if (paused_) {
        paused_ = false;
        ui->btn_pause_resume->setText("PAUSE");
    }

    ui->statusbar->showMessage("Emergency Stop 指令已下发", 3000);
    qDebug() << "Estop command sent to MODBUS[" << kRegEventLevel0
             << "] = " << kEventEstop;
}


void MainWindow::on_btn_error_reset_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }

    Result ret = ctx_->motionService()->sendErrorReset();
    if (!ret.ok) {
        QMessageBox::warning(this, "Error Reset 失败", ret.message);
        return;
    }

    ui->statusbar->showMessage("Error Reset 指令已下发", 3000);
    qDebug() << "ErrorReset command sent to MODBUS[" << kRegEventLevel0
             << "] = " << kEventErrorReset;
}


// ── Robot Mode ──────────────────────────────────────────

void MainWindow::on_btn_robo_mode_in_clicked()
{
    if (!ctx_->connectionService()->isConnected()) {
        QMessageBox::warning(this, "错误", "控制器未连接");
        return;
    }
    enterRobotMode();
}

void MainWindow::on_btn_robo_mode_out_clicked()
{
    // 如果 Cart Jog / Trace / Ctrl Trace 模式处于激活状态，先退出
    if (currentMotionMode_ == MotionMode::CartJog
        || currentMotionMode_ == MotionMode::Trace
        || currentMotionMode_ == MotionMode::CtrlTrace) {
        exitCurrentMode();
    }
    exitRobotMode();
}

