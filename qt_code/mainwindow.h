#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

#include "./app/AppContext.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum class MotionMode {
    None,
    DirectJoint,
    CartJog,
    Trace,
    CtrlTrace
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(AppContext* context, QWidget *parent = nullptr);
    ~MainWindow();

    void Init();
    void ip_Scan();
    void connect_Ether();
    void closeEther();
    void dat_to_xlsx();
    void xlsx_to_dat();
    void motion_cmd();

private slots:
    // Connection
    void on_btn_ip_scan_clicked();
    void on_btn_connect_controller_clicked();
    void on_btn_disconnect_controller_clicked();

    // Home
    void on_btn_home_clicked();

    // Direct Joint
    void on_btn_direct_joint_enter_clicked();
    void on_btn_direct_joint_exit_clicked();
    void on_btn_direct_joint_send_clicked();

    // Cart Jog
    void on_btn_jog_enter_clicked();
    void on_btn_jog_exit_clicked();
    void on_btn_jog_send_clicked();

    // Trace
    void on_btn_trace_enter_clicked();
    void on_btn_trace_exit_clicked();
    void on_btn_thread_open_clicked();
    void on_btn_thread_close_clicked();
    void on_btn_trace_send_clicked();

    // Ctrl Trace（轨迹控制）
    void on_btn_ctrl_enter_clicked();
    void on_btn_ctrl_exit_clicked();
    void on_btn_ctrl_send_clicked();

    // End Tool（末端工具）
    void on_btn_revolution_clicked();
    void on_btn_rotation_clicked();

    // Control
    void on_btn_pause_resume_clicked();
    void on_btn_stop_clicked();
    void on_btn_emergency_stop_clicked();
    void on_btn_error_reset_clicked();

    // Data
    void on_btn_trace_test_clicked();
    void on_btn_trace_to_xlsx_clicked();
    void on_btn_xlsx_to_dat_clicked();
    void on_btn_robo_mode_in_clicked();
    void on_btn_robo_mode_out_clicked();
    void on_btn_data_save_clicked();

private:
    // ── 模式切换 ──────────────────────────────────────
    bool canEnterMode(MotionMode mode);
    void enterMode(MotionMode mode);
    void exitCurrentMode();
    void resetMotionModeUi();
    void updateMotionModeDisplay();
    void updateTraceButtonStates();
    void updateCtrlButtonStates();

    // ── Robot Mode ──────────────────────────────────────
    void enterRobotMode();
    void exitRobotMode();
    void updateRobotModeButtons();

    // ── 运动指令 ──────────────────────────────────────
    void sendDirectJointCmd();
    void sendCartJogCmd();
    void sendHomeCmd();
    void openTraceThread();
    void closeTraceThread();
    void sendTraceFile();
    void sendCtrlTraceFile();

    // ── 末端工具 ──────────────────────────────────────
    void updateRevolutionUi();
    void updateRotationUi();
    void resetToolUi();

    // ── 全局控制 ──────────────────────────────────────
    void sendPause();
    void sendResume();
    bool paused_ = false;

    // ── 数据保存 ──────────────────────────────────────
    void stopDataSave();  // 停止录制并复位 UI（关闭文件、按钮/状态复原）

    Ui::MainWindow *ui;
    AppContext* ctx_;

    MotionMode currentMotionMode_ = MotionMode::None;
    bool traceThreadOpened_ = false;
    bool robotModeActive_ = false;
    bool revolutionRunning_ = false;
    bool rotationRunning_ = false;

    // ── 数据保存 ──────────────────────────────────────
    bool dataSaving_ = false;
    QFile dataSaveFile_;
    QTextStream dataSaveStream_;
    QElapsedTimer dataSaveFlushTimer_;  // flush 节流：每秒落盘一次
};

#endif // MAINWINDOW_H
