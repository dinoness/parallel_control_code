%% generate_trajectory_linear.m
% 生成五自由度并联机器人直线轨迹二进制文件
% 轨迹：从 (0, 0, -800) 直线运动到 (0, -190, -725)
% phi=0, theta=0
% 单位：x,y,z 输出为微米(μm)；phi,theta 输出为度(°)
% 数据格式：小端序单精度浮点，每组 7 个 float [cmd, x, y, z, phi, theta, ticks]

clc; clear; close all;

%% 参数设置
start_pos = [0, 0, -800];    % 起点 (mm)
end_pos   = [0, -190, -725];    % 终点 (mm)
phi       = 0;                  % 绕 Z 轴转角 (°)
theta     = 0;                  % 绕 Y 轴转角 (°)

dt        = 1e-3;               % 指令周期 (s)
T_total   = 30;                 % 总时长 (s)

filename  = 'trajectory_linear_30s.dat'; % 输出文件名

%% 生成时间序列与插值点
N = round(T_total / dt) + 1;    % 总点数（含起点）
t = linspace(0, T_total, N);    % 时间向量

% 对各轴做线性插值 (mm -> μm)
x_vec = linspace(start_pos(1), end_pos(1), N) * 1e3;  % μm
y_vec = linspace(start_pos(2), end_pos(2), N) * 1e3;  % μm
z_vec = linspace(start_pos(3), end_pos(3), N) * 1e3;  % μm

phi_vec   = ones(1, N) * phi;
theta_vec = ones(1, N) * theta;

% cmd: 第一帧设为 2.0，其余设为 10.0（与原始 01.dat 格式保持一致）
cmd_vec = [2.0, ones(1, N-1) * 10.0];

% ticks: 全部设为 50.0（与原始 01.dat 格式保持一致）
ticks_vec = ones(1, N) * 50.0;

%% 组装数据矩阵 (7 x N)
data = [cmd_vec;
        x_vec;
        y_vec;
        z_vec;
        phi_vec;
        theta_vec;
        ticks_vec];

%% 写入二进制文件（小端序单精度浮点）
fid = fopen(filename, 'wb');
if fid == -1
    error('无法创建文件: %s', filename);
end

fwrite(fid, data, 'single', 'l');  % 'single'=float32, 'l'=little-endian
fclose(fid);

fprintf('轨迹文件已生成: %s\n', filename);
fprintf('总点数: %d, 每组 %d bytes, 文件大小: %.2f KB\n', ...
    N, 7*4, N*7*4/1024);
fprintf('起点: x=%.1f, y=%.1f, z=%.1f (μm)\n', x_vec(1), y_vec(1), z_vec(1));
fprintf('终点: x=%.1f, y=%.1f, z=%.1f (μm)\n', x_vec(end), y_vec(end), z_vec(end));

%% 可视化轨迹
figure('Name', '直线轨迹曲线', 'NumberTitle', 'off');
subplot(2,1,1);
plot(t, x_vec/1e3, 'r', t, y_vec/1e3, 'g', t, z_vec/1e3, 'b', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('位移 (mm)');
legend('x','y','z'); grid on;
title('各轴位移曲线');

subplot(2,1,2);
plot3(x_vec/1e3, y_vec/1e3, z_vec/1e3, 'b-', 'LineWidth', 1.5);
hold on;
plot3(start_pos(1), start_pos(2), start_pos(3), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
plot3(end_pos(1), end_pos(2), end_pos(3), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('x (mm)'); ylabel('y (mm)'); zlabel('z (mm)');
legend('轨迹','起点','终点'); grid on; axis equal;
title('三维轨迹');
