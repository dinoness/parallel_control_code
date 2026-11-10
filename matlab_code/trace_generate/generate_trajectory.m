%% generate_trajectory.m
% 生成五自由度并联机器人轨迹二进制文件
% 轨迹分三段：
%   1) 0~5s：  从 (0, 0, -800) 直线运动到 (20, 0, -800)
%   2) 5~15s： 以 (0, 0, -800) 为圆心，绕 z 轴右手螺旋方向旋转 90°
%   3) 15~30s：从圆弧终点直线回到 (0, 0, -800)
% 单位：x,y,z 输出为微米(μm)；phi,theta 输出为度(°)
% 数据格式：小端序单精度浮点，每组 7 个 float [cmd, x, y, z, phi, theta, ticks]

clc; clear; close all;

%% 参数设置
dt        = 1e-3;               % 指令周期 (s)
T_total   = 30;                 % 总时长 (s)

start_pos = [0, 0, -800];       % 起点 (mm)
mid1_pos  = [20, 0, -800];      % 第一段终点 (mm)
center    = [0, 0, -800];       % 圆弧圆心 (mm)
R         = 20;                 % 圆弧半径 (mm)

phi       = 0;                  % 绕 Z 轴转角 (°)
theta     = 0;                  % 绕 Y 轴转角 (°)

filename  = 'trajectory_30s_cir.dat'; % 输出文件名

%% 时间分段
T1 = 5;     % 第一段时长 (s)
T2 = 10;    % 第二段时长 (s)
T3 = 15;    % 第三段时长 (s)

N_total = round(T_total / dt) + 1;   % 总点数（含起点）
N1 = round(T1 / dt);                 % 第一段步数（不含起点）
N2 = round(T2 / dt);                 % 第二段步数
N3 = round(T3 / dt);                 % 第三段步数

% 校验总点数
assert(N1 + N2 + N3 == N_total - 1, '分段点数总和不等于总点数');

t = linspace(0, T_total, N_total);   % 时间向量

%% 生成各段轨迹
% ---------- 第一段：直线 (0~5 s) ----------
N1_pts = N1 + 1;  % 包含起点
t1 = linspace(0, T1, N1_pts);
x1 = linspace(start_pos(1), mid1_pos(1), N1_pts);
y1 = linspace(start_pos(2), mid1_pos(2), N1_pts);
z1 = linspace(start_pos(3), mid1_pos(3), N1_pts);

% ---------- 第二段：圆弧 (5~15 s) ----------
% 右手螺旋绕 +z 轴：角度从 0° 增加到 90°
N2_pts = N2 + 1;  % 包含第二段起点（与第一段终点重合）
t2 = linspace(T1, T1+T2, N2_pts);
angle = linspace(0, pi/2, N2_pts);   % 弧度，0 ~ 90°

x2 = center(1) + R * cos(angle);
y2 = center(2) + R * sin(angle);
z2 = ones(1, N2_pts) * center(3);

% ---------- 第三段：直线 (15~30 s) ----------
N3_pts = N3 + 1;  % 包含第三段起点（与第二段终点重合）
t3 = linspace(T1+T2, T_total, N3_pts);
end_pos = [center(1) + R * cos(pi/2), center(2) + R * sin(pi/2), center(3)];  % (0, 20, -795.09)

x3 = linspace(end_pos(1), start_pos(1), N3_pts);
y3 = linspace(end_pos(2), start_pos(2), N3_pts);
z3 = linspace(end_pos(3), start_pos(3), N3_pts);

%% 拼接轨迹（去除重复连接点）
x_vec = [x1, x2(2:end), x3(2:end)];
y_vec = [y1, y2(2:end), y3(2:end)];
z_vec = [z1, z2(2:end), z3(2:end)];

% 单位转换为 μm
x_vec = x_vec * 1e3;
y_vec = y_vec * 1e3;
z_vec = z_vec * 1e3;

% phi, theta
phi_vec   = ones(1, N_total) * phi;
theta_vec = ones(1, N_total) * theta;

% cmd: 第一帧设为 2.0，其余设为 10.0
cmd_vec = [2.0, ones(1, N_total-1) * 10.0];

% ticks: 全部设为 50.0
ticks_vec = ones(1, N_total) * 50.0;

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
fprintf('第一段 (0~5s):   起点 (%.1f, %.1f, %.1f) -> 终点 (%.1f, %.1f, %.1f) (μm)\n', ...
    x_vec(1), y_vec(1), z_vec(1), x_vec(N1_pts), y_vec(N1_pts), z_vec(N1_pts));
fprintf('第二段 (5~15s):  圆弧圆心 (%.1f, %.1f, %.1f), 半径 %.1f mm, 旋转 90° (μm)\n', ...
    center(1)*1e3, center(2)*1e3, center(3)*1e3, R);
fprintf('第三段 (15~30s): 起点 (%.1f, %.1f, %.1f) -> 终点 (%.1f, %.1f, %.1f) (μm)\n', ...
    x_vec(N1_pts+N2_pts-1), y_vec(N1_pts+N2_pts-1), z_vec(N1_pts+N2_pts-1), ...
    x_vec(end), y_vec(end), z_vec(end));

%% 可视化轨迹
figure('Name', '轨迹曲线', 'NumberTitle', 'off');

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
plot3(start_pos(1), start_pos(2), start_pos(3), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
plot3(mid1_pos(1), mid1_pos(2), mid1_pos(3), 'mo', 'MarkerSize', 8, 'MarkerFaceColor', 'm');
plot3(end_pos(1), end_pos(2), end_pos(3), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('x (mm)'); ylabel('y (mm)'); zlabel('z (mm)');
legend('轨迹','起点','圆弧起点/第一段终点','圆弧终点'); grid on; axis equal;
title('三维轨迹');
view(3);
