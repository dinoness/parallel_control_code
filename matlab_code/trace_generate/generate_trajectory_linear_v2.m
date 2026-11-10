%% generate_trajectory_linear_v2.m
% Generate linear motion trajectory for 5-DOF parallel robot
% Trajectory: from (0, 0, -800) to (-50, -170, -725)
% Duration: 30s, time step: 0.001s
% End with 1 extra hold-at-end group
% Units: x,y,z output in um; phi,theta output in degrees
% Data format: little-endian single-precision float, 7 floats/group [cmd, x, y, z, phi, theta, ticks]

clc; clear; close all;

%% Parameters
start_pos = [0, 0, -800];        % start point (mm)
end_pos   = [-50, -170, -725];   % end point (mm)
phi       = 0;                    % Z-axis rotation angle (deg)
theta     = 0;                    % Y-axis rotation angle (deg)

dt        = 1e-3;                % time step (s)
T_total   = 30;                  % total duration (s)

filename  = 'trajectory_linear_30s.dat';  % output filename

%% Generate time series and interpolation points
N_motion = round(T_total / dt) + 1;   % motion points (including start)
t = linspace(0, T_total, N_motion);   % time vector

% Linear interpolation on each axis (mm -> um)
x_vec = linspace(start_pos(1), end_pos(1), N_motion) * 1e3;  % um
y_vec = linspace(start_pos(2), end_pos(2), N_motion) * 1e3;  % um
z_vec = linspace(start_pos(3), end_pos(3), N_motion) * 1e3;  % um

% Append 1 extra hold-at-end group (maintaining final position)
x_vec(end+1)    = x_vec(end);
y_vec(end+1)    = y_vec(end);
z_vec(end+1)    = z_vec(end);

N_total = N_motion + 1;   % total points = motion + 1 hold

phi_vec   = ones(1, N_total) * phi;
theta_vec = ones(1, N_total) * theta;

% cmd: first frame = 2.0, rest = 10.0
cmd_vec = [2.0, ones(1, N_total-1) * 10.0];

% ticks: all = 50.0
ticks_vec = ones(1, N_total) * 50.0;

%% Assemble data matrix (7 x N)
data = [cmd_vec;
        x_vec;
        y_vec;
        z_vec;
        phi_vec;
        theta_vec;
        ticks_vec];

%% Write binary file (little-endian single-precision float)
fid = fopen(filename, 'wb');
if fid == -1
    error('Cannot create file: %s', filename);
end

fwrite(fid, data, 'single', 'l');  % 'single'=float32, 'l'=little-endian
fclose(fid);

fprintf('Trajectory file generated: %s\n', filename);
fprintf('Total points: %d (motion: %d, hold: 1)\n', N_total, N_motion);
fprintf('File size: %.2f KB (%d bytes)\n', N_total*7*4/1024, N_total*7*4);
fprintf('Group size: %d bytes (7 x float32)\n', 7*4);
fprintf('Start: x=%.1f, y=%.1f, z=%.1f (um)\n', x_vec(1), y_vec(1), z_vec(1));
fprintf('End:   x=%.1f, y=%.1f, z=%.1f (um)\n', x_vec(end), y_vec(end), z_vec(end));

%% Visualize trajectory
figure('Name', 'Linear Trajectory', 'NumberTitle', 'off');

subplot(2,1,1);
hold on;
plot(1:N_motion, x_vec(1:N_motion)/1e3, 'r', 'LineWidth', 1.2);
plot(1:N_motion, y_vec(1:N_motion)/1e3, 'g', 'LineWidth', 1.2);
plot(1:N_motion, z_vec(1:N_motion)/1e3, 'b', 'LineWidth', 1.2);
xlabel('Time step'); ylabel('Displacement (mm)');
legend('x','y','z'); grid on;
title('Axis displacement curves');

subplot(2,1,2);
plot3(x_vec/1e3, y_vec/1e3, z_vec/1e3, 'b-', 'LineWidth', 1.5);
hold on;
plot3(start_pos(1), start_pos(2), start_pos(3), 'go', 'MarkerSize', 10, 'MarkerFaceColor', 'g');
plot3(end_pos(1), end_pos(2), end_pos(3), 'ro', 'MarkerSize', 10, 'MarkerFaceColor', 'r');
xlabel('x (mm)'); ylabel('y (mm)'); zlabel('z (mm)');
legend('Trajectory','Start','End'); grid on; axis equal;
title('3D Trajectory');
view(3);
