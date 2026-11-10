%% generate_trajectory_3seg_inc.m
% 生成五自由度并联机器人三段直线轨迹二进制文件（增量模式）
% 轨迹分三段，首尾相连形成闭合三角形：
%   1) 0~20s：  从 (0, 0, -800) 直线运动到 (-50, -30, -820)
%   2) 20~40s： 从 (-50, -30, -820) 直线运动到 (50, -30, -780)
%   3) 40~60s： 从 (50, -30, -780) 直线运动回到 (0, 0, -800)
% 每段匀速运动，总时间 60s
% 与 generate_trajectory_3seg.m 的区别：
%   - cmd 全部设为 9（增量模式指令）
%   - x,y,z 输出为相对于上一帧的增量，而不是绝对位置
%   - phi,theta 同样输出增量（本轨迹中恒为 0）
%   - 数据末尾追加一行全为 0 的结束行
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
T_seg     = 40;             % 每段时长 (s)
T_total   = 3 * T_seg;      % 总时长 60s

filename  = 'trajectory_3seg_60s_inc.dat'; % 输出文件名

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

% phi, theta（绝对量）
phi_vec   = ones(1, N_total) * phi;
theta_vec = ones(1, N_total) * theta;

%% 绝对量转增量（第一帧增量设为 0，表示从初始位姿出发）
dx_vec     = [0, diff(x_vec)];
dy_vec     = [0, diff(y_vec)];
dz_vec     = [0, diff(z_vec)];
dphi_vec   = [0, diff(phi_vec)];
dtheta_vec = [0, diff(theta_vec)];

% 单位转换为 μm
dx_vec = dx_vec * 1e3;
dy_vec = dy_vec * 1e3;
dz_vec = dz_vec * 1e3;

% cmd: 全部设为 9（增量模式指令）
cmd_vec = ones(1, N_total) * 9.0;

% ticks: 全部设为 1.0
ticks_vec = ones(1, N_total) * 1.0;

%% 组装数据矩阵 (7 x N)
data = [cmd_vec;
        dx_vec;
        dy_vec;
        dz_vec;
        dphi_vec;
        dtheta_vec;
        ticks_vec];

%% 末尾追加一行全为 0 的结束行
data = [data, zeros(7, 1)];

%% 写入二进制文件（小端序单精度浮点）
fid = fopen(filename, 'wb');
if fid == -1
    error('无法创建文件: %s', filename);
end

fwrite(fid, data, 'single', 'l');  % 'single'=float32, 'l'=little-endian
fclose(fid);

fprintf('轨迹文件已生成: %s\n', filename);
fprintf('总点数: %d (含末尾全 0 行), 每组 %d bytes, 文件大小: %.2f KB\n', ...
    N_total + 1, 7*4, (N_total + 1)*7*4/1024);
fprintf('每帧增量: dx = %.4f μm (段1), %.4f μm (段2), %.4f μm (段3)\n', ...
    dx_vec(2)*1e0, dx_vec(N_seg+1)*1e0, dx_vec(2*N_seg)*1e0);
fprintf('增量累计校验: 增量总和 = (%.4f, %.4f, %.4f) mm (闭合轨迹应回到起点，总和为 0)\n', ...
    sum(dx_vec)/1e3, sum(dy_vec)/1e3, sum(dz_vec)/1e3);

%% 可视化轨迹（通过增量累计还原绝对轨迹）
x_abs = cumsum(dx_vec) / 1e3 + p1(1);
y_abs = cumsum(dy_vec) / 1e3 + p1(2);
z_abs = cumsum(dz_vec) / 1e3 + p1(3);

figure('Name', '三段直线轨迹（增量模式）', 'NumberTitle', 'off');

subplot(2,2,1);
plot(t, x_abs, 'r', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('x (mm)'); grid on; title('x 轴位移（增量累计还原）');

subplot(2,2,2);
plot(t, y_abs, 'g', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('y (mm)'); grid on; title('y 轴位移（增量累计还原）');

subplot(2,2,3);
plot(t, z_abs, 'b', 'LineWidth', 1.2);
xlabel('时间 (s)'); ylabel('z (mm)'); grid on; title('z 轴位移（增量累计还原）');

subplot(2,2,4);
plot3(x_abs, y_abs, z_abs, 'b-', 'LineWidth', 1.5);
hold on;
plot3(p1(1), p1(2), p1(3), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
plot3(p2(1), p2(2), p2(3), 'mo', 'MarkerSize', 8, 'MarkerFaceColor', 'm');
plot3(p3(1), p3(2), p3(3), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('x (mm)'); ylabel('y (mm)'); zlabel('z (mm)');
legend('轨迹','起点 p1','中间点 p2','终点 p3'); grid on; axis equal;
title('三维轨迹');
view(3);
