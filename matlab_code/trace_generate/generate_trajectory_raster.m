%% generate_trajectory_raster.m
% 生成五自由度并联机器人光栅轨迹二进制文件
% 轨迹顺序：
%   1) MOVEABS 到 (50, 50, -800)
%   2) 在 z=-800 平面进行光栅扫描：
%      - x 从 50 到 -50，相邻扫描线间隔 10 mm
%      - 沿 y 方向在 50 和 -50 之间蛇形往返
%      - 光栅终点为 (-50, -50, -800)
%   3) MOVEABS 到 (0, 0, -750)
% 全程 phi=0, theta=0，光栅进给速率为 5 mm/s
% 单位：x,y,z 输出为微米(μm)；phi,theta 输出为度(°)
% 数据格式：小端序单精度浮点，每组 7 个 float [cmd, x, y, z, phi, theta, ticks]

clc; clear; close all;

%% 参数设置
start_pos  = [50, 50, -800];     % 光栅起点 (mm)
raster_end = [-50, -50, -800];   % 光栅终点 (mm)
final_pos  = [0, 0, -750];       % 最终位置 (mm)

phi   = 0;                       % 绕 Z 轴转角 (°)
theta = 0;                       % 绕 Y 轴转角 (°)

feed_rate = 5;                   % 光栅进给速率 (mm/s)
dt        = 1e-3;                % PT 指令周期 (s)
step_mm   = feed_rate * dt;      % 每条 PT 指令的位移 (mm)
x_columns = 50:-10:-50;          % 各条光栅扫描线的 x 坐标

script_dir = fileparts(mfilename('fullpath'));
output_dir = fullfile(script_dir, 'data');
if ~exist(output_dir, 'dir')
    mkdir(output_dir);
end
filename = fullfile(output_dir, 'trajectory_raster_5mms.dat');

%% 第一段：MOVEABS 到光栅起点
% MOVEABS 不使用第 7 字段，因此设为 0
start_cmd = [2; start_pos(:) * 1e3; phi; theta; 0];

%% 第二段：生成蛇形光栅轨迹
raster_data = zeros(7, 0);
current_pos = start_pos;

for k = 1:numel(x_columns)
    % 奇数列向 -Y 扫描，偶数列向 +Y 扫描
    if mod(k, 2) == 1
        target_y = -50;
    else
        target_y = 50;
    end

    target_pos = [x_columns(k), target_y, -800];
    segment = MakePTABSLine(current_pos, target_pos, step_mm, phi, theta);
    raster_data = [raster_data, segment]; %#ok<AGROW>
    current_pos = target_pos;

    % 扫描线之间沿 X 方向移动 10 mm，最后一列后不再移动
    if k < numel(x_columns)
        target_pos = [x_columns(k) - 10, target_y, -800];
        segment = MakePTABSLine(current_pos, target_pos, step_mm, phi, theta);
        raster_data = [raster_data, segment]; %#ok<AGROW>
        current_pos = target_pos;
    end
end

assert(norm(current_pos - raster_end) < 1e-9, '光栅终点与设定值不一致');

%% 第三段：MOVEABS 到最终位置
final_cmd = [2; final_pos(:) * 1e3; phi; theta; 0];

%% 拼接并写入二进制文件
data = [start_cmd, raster_data, final_cmd];
N_total = size(data, 2);

fid = fopen(filename, 'wb');
if fid == -1
    error('无法创建文件: %s', filename);
end
fwrite(fid, data, 'single', 'l');  % single=float32，l=little-endian
fclose(fid);

raster_length = (numel(x_columns) * 100 + (numel(x_columns) - 1) * 10);
raster_time = raster_length / feed_rate;

fprintf('轨迹文件已生成: %s\n', filename);
fprintf('总点数: %d（光栅点数: %d）\n', N_total, size(raster_data, 2));
fprintf('光栅长度: %.1f mm，进给速率: %.1f mm/s，扫描时间: %.1f s\n', ...
    raster_length, feed_rate, raster_time);
fprintf('文件大小: %.2f KB（%d bytes）\n', N_total * 7 * 4 / 1024, N_total * 7 * 4);

%% 绘制 XY 光栅路径
figure('Name', '光栅轨迹', 'NumberTitle', 'off');
plot(raster_data(2, :) / 1e3, raster_data(3, :) / 1e3, 'b-', 'LineWidth', 1.2);
hold on;
plot(start_pos(1), start_pos(2), 'go', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
plot(raster_end(1), raster_end(2), 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('X (mm)'); ylabel('Y (mm)');
title('XY 平面蛇形光栅轨迹');
legend('光栅路径', '起点', '终点', 'Location', 'best');
grid on; axis equal;

%% 生成一段等速 MOVE_PTABS 直线轨迹（不包含重复起点，包含终点）
function segment = MakePTABSLine(start_pos, end_pos, step_mm, phi, theta)
    distance = norm(end_pos - start_pos);
    N = round(distance / step_mm);
    assert(abs(distance - N * step_mm) < 1e-9, ...
        '线段长度不能被单步位移整除');

    ratio = (1:N) / N;
    positions = repmat(start_pos(:), 1, N) + ...
        (end_pos(:) - start_pos(:)) * ratio;  % 3 x N，单位 mm

    segment = zeros(7, N);
    segment(1, :) = 10;              % MOVE_PTABS
    segment(2:4, :) = positions * 1e3;  % mm -> μm
    segment(5, :) = phi;
    segment(6, :) = theta;
    segment(7, :) = 1;               % ticks=1，即 1 ms
end
