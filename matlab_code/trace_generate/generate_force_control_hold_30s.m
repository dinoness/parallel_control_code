%% generate_force_control_hold_30s.m
% 生成五自由度并联机器人力控算法测试轨迹二进制文件
% 轨迹顺序：
%   1) MOVEABS 到 (0, 0, -800)
%   2) FORCE_CFG 启动力控
%   3) 连续 30 s 将理论位置保持为 (0, 0, -800)
%   4) FORCE_OFF 结束力控
%   5) MOVEABS 到 (0, 0, -750)
% 全程 phi=0, theta=0
%
% 注意：本文件必须通过 CTRL 混合轨迹执行，普通 TRAJ 不接受 FORCE_CFG/FORCE_OFF。
% 单位：x,y,z 输出为微米(μm)；phi,theta 输出为度(°)
% 数据格式：小端序单精度浮点，每组 7 个 float [cmd, p1, p2, p3, p4, p5, p7]

clc; clear; close all;

%% 位姿与时间参数
start_pos = [0, 0, -793];        % 力控理论位置 (mm)
final_pos = [0, 0, -720];        % 最终位置 (mm)
phi       = 0;                   % 绕 Z 轴转角 (°)
theta     = 0;                   % 绕 Y 轴转角 (°)
hold_time = 30;                  % 力控保持时间 (s)
dt        = 1e-3;                % PT 指令周期 (s)

%% 力控参数（上机前请根据传感器标定和试验要求确认）
force_mode        = 2;           % 1=接触搜索，2=恒力保持
force_target_N    = 5;           % 目标力 (N)，示例值
force_contact_N   = 2;           % 接触判定阈值及接触前参考力 (N)
force_ramp_step_N = 0.001;       % 每控制周期的目标力变化量 (N/周期)，必须大于 0
max_adjust_um     = 50000.0;     % 本次轨迹允许的最大 Z 修正量 (μm)

%% 参数校验
assert(hold_time > 0 && dt > 0, '保持时间和指令周期必须大于 0');
assert(force_mode == 1 || force_mode == 2, '力控模式只能为 1 或 2');
assert(force_target_N >= 0, '目标力不能为负数');
assert(force_ramp_step_N > 0, '每周期的目标力变化量必须大于 0');
assert(max_adjust_um > 0, '最大 Z 修正量必须大于 0');
if force_mode == 1
    assert(force_contact_N > 0, '接触搜索模式的接触判定阈值必须大于 0');
end

N_hold = round(hold_time / dt);
assert(abs(N_hold * dt - hold_time) < 1e-9, ...
    '保持时间必须是指令周期的整数倍');

%% 构造轨迹指令
% 1) MOVEABS 到力控理论位置；MOVEABS 不使用第 7 字段，因此设为 0
start_cmd = [2; start_pos(:) * 1e3; phi; theta; 0];

% 2) FORCE_CFG = [30, mode, target_N, contact_N, ramp_step_N, max_adjust_um, reserved]
force_cfg_cmd = [30; force_mode; force_target_N; force_contact_N; ...
    force_ramp_step_N; max_adjust_um; 0];

% 3) 力控段只接受 PT/PTABS、FORCE_CFG、FORCE_OFF，不能用 MOVE_DELAY 保持。
%    每个保持点 ticks=1，重复 N_hold 次对应 30 s 的名义理论位置流。
hold_cmd_template = [10; start_pos(:) * 1e3; phi; theta; 1];
hold_cmds = repmat(hold_cmd_template, 1, N_hold);

% 4) FORCE_OFF 结束后，控制器会等待带修正量的运动完成，再恢复普通段
force_off_cmd = [31; zeros(6, 1)];

% 5) MOVEABS 到最终位置
final_cmd = [2; final_pos(:) * 1e3; phi; theta; 0];

data = [start_cmd, force_cfg_cmd, hold_cmds, force_off_cmd, final_cmd];
N_total = size(data, 2);

%% 写入 data 文件夹
script_dir = fileparts(mfilename('fullpath'));
output_dir = fullfile(script_dir, 'data');
if ~exist(output_dir, 'dir')
    mkdir(output_dir);
end
filename = fullfile(output_dir, 'force_control_hold_30s.dat');

fid = fopen(filename, 'wb');
if fid == -1
    error('无法创建文件: %s', filename);
end
fwrite(fid, data, 'single', 'l');  % single=float32，l=little-endian
fclose(fid);

fprintf('轨迹文件已生成: %s\n', filename);
fprintf('总记录数: %d（保持点: %d）\n', N_total, N_hold);
fprintf('力控模式: %d，目标力: %.3f N，保持时间: %.1f s\n', ...
    force_mode, force_target_N, N_hold * dt);
fprintf('理论位置: (%.1f, %.1f, %.1f) mm，姿态: (%.1f, %.1f) deg\n', ...
    start_pos, phi, theta);
fprintf('文件大小: %.2f KB（%d bytes）\n', ...
    N_total * 7 * 4 / 1024, N_total * 7 * 4);
