%% generate_trajectory_3seg.m
% 生成五自由度并联机器人三段直线轨迹二进制文件
% 轨迹分三段，首尾相连形成闭合三角形：
%   1) 0~20s：  从 (0, 0, -800) 直线运动到 (-50, -30, -820)
%   2) 20~40s： 从 (-50, -30, -820) 直线运动到 (50, -30, -780)
%   3) 40~60s： 从 (50, -30, -780) 直线运动回到 (0, 0, -800)
% 每段匀速运动，总时间 60s
% 单位：x,y,z 输出为微米(μm)；phi,theta 输出为度(°)
% 数据格式：小端序单精度浮点，每组 7 个 float [cmd, x, y, z, phi, theta, ticks]

clc; clear; close all;

%% 参数设置
p1 = [0, 0, -800];          % 第一个点 (mm)
p2 = [-50, -30, -820];      % 第二个点 (mm)
p3 = [50, -30, -780];       % 第三个点 (mm)

phi       = 0;              % 绕 Z 轴转角 (°)
theta     = 0;              % 绕 Y 轴转角 (°)

dt        = 1e-3;           % 指令周期 (s)
T_seg     = 10;             % 每段时长 (s)
T_total   = 3 * T_seg;      % 总时长 60s

filename  = 'trajectory_3seg_60s.dat'; % 输出文件名

%% 生成各段轨迹
N_seg = round(T_seg / dt) + 1;  % 每段点数（含端点）

% ---------- 第一段：p1 -> p2 (0~20s) ----------
t1 = linspace(0, T_seg, N_seg);
x1 = linspace(p1(1), p2(1), N_seg);
y1 = linspace(p1(2), p2(2), N_seg);
z1 = linspace(p1(3), p2(3), N_seg);

% ---------- 第二段：p2 -> p3 (20~40s) ----------
t2 = linspace(T_seg, 2*T_seg, N_seg);
x2 = linspace(p2(1), p3(1), N_seg);
y2 = linspace(p2(2), p3(2), N_seg);
z2 = linspace(p2(3), p3(3), N_seg);

% ---------- 第三段：p3 -> p1 (40~60s) ----------
t3 = linspace(2*T_seg, T_total, N_seg);
x3 = linspace(p3(1), p1(1), N_seg);
y3 = linspace(p3(2), p1(2), N_seg);
z3 = linspace(p3(3), p1(3), N_seg);

%% 拼接轨迹（去除重复连接点）
x_vec = [x1, x2(2:end), x3(2:end)];
y_vec = [y1, y2(2:end), y3(2:end)];
z_vec = [z1, z2(2:end), z3(2:end)];

N_total = length(x_vec);
t = linspace(0, T_total, N_total);

% 单位转换为 μm
x_vec = x_vec * 1e3;
y_vec = y_vec * 1e3;
z_vec = z_vec * 1e3;

% phi, theta
phi_vec   = ones(1, N_total) * phi;
theta_vec = ones(1, N_total) * theta;

% cmd: 第一帧设为 2.0，其余设为 10.0
cmd_vec = [10, ones(1, N_total-1) * 10.0];

% ticks: 全部设为 1.0
ticks_vec = ones(1, N_total) * 1.0;

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
    N_total, 7*4, N_total*7*4/1024);
fprintf('第一段 (0~20s):   起点 (%.1f, %.1f, %.1f) -> 终点 (%.1f, %.1f, %.1f) (μm)\n', ...
    x_vec(1), y_vec(1), z_vec(1), x_vec(N_seg), y_vec(N_seg), z_vec(N_seg));
fprintf('第二段 (20~40s):  起点 (%.1f, %.1f, %.1f) -> 终点 (%.1f, %.1f, %.1f) (μm)\n', ...
    x_vec(N_seg), y_vec(N_seg), z_vec(N_seg), ...
    x_vec(2*N_seg-1), y_vec(2*N_seg-1), z_vec(2*N_seg-1));
fprintf('第三段 (40~60s): 起点 (%.1f, %.1f, %.1f) -> 终点 (%.1f, %.1f, %.1f) (μm)\n', ...
    x_vec(2*N_seg-1), y_vec(2*N_seg-1), z_vec(2*N_seg-1), ...
    x_vec(end), y_vec(end), z_vec(end));

%% 可视化轨迹
figure('Name', '三段直线轨迹', 'NumberTitle', 'off');

subplot(2,2,1);
plot(t, x_vec/1e3, 'r', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('x (mm)'); grid on; title('x 轴位移');

subplot(2,2,2);
plot(t, y_vec/1e3, 'g', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('y (mm)'); grid on; title('y 轴位移');

subplot(2,2,3);
plot(t, z_vec/1e3, 'b', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('z (mm)'); grid on; title('z 轴位移');

subplot(2,2,4);
plot3(x_vec/1e3, y_vec/1e3, z_vec/1e3, 'b-', 'LineWidth', 1.5);
hold on;
plot3(p1(1), p1(2), p1(3), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
plot3(p2(1), p2(2), p2(3), 'mo', 'MarkerSize', 8, 'MarkerFaceColor', 'm');
plot3(p3(1), p3(2), p3(3), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('x (mm)'); ylabel('y (mm)'); zlabel('z (mm)');
legend('轨迹','起点 p1','中间点 p2','终点 p3'); grid on; axis equal;
title('三维轨迹');
view(3);
