clear all; clc;
% ROS 2 - Connection Test
global data_1;
global data_2;
data_1 = [];
data_2 = [];
% bashfile_1 = 'gnome-terminal -- bash -c "bash ~/matlab_gui_experiment/ros2_node_ws_test/run_publisher.sh; exec bash"';
% bashfile_2 = 'gnome-terminal -- bash -c "bash ~/matlab_gui_experiment/ros2_node_ws_test/run_publisher_2.sh; exec bash"';
% [status_bash_1, ~] = system(bashfile_1);
% [status_bash_2, ~] = system(bashfile_2);
% launch_file = 'gnome-terminal -- bash -c "bash ~/matlab_gui_experiment/ros2_node_ws_test/launch_nodes.sh; exec bash"';
launch_file_1 = 'gnome-terminal -- bash -c "bash ~/experiment_grinding_nose/sensors_experiment/launch_ati.sh; exec bash"';
launch_file_2 = 'gnome-terminal -- bash -c "bash ~/experiment_grinding_nose/sensors_experiment/run_aurora.sh; exec bash"';
[status_bash_1, ~] = system(launch_file_1);
[status_bash_2, ~] = system(launch_file_2);

sensors_read = ros2node('/sensors_read');

% topic_name_1 = '/topic_1';
% topic_name_2 = '/topic_2';
topic_name_1 = '/force_torque_sensor_broadcaster/wrench';
topic_name_2 = '/aurora_transform';
pause(5); % wait until the topic is in the network before creating the subscriber
subs_1 = ros2subscriber(sensors_read, topic_name_1, @callback_1);
subs_2 = ros2subscriber(sensors_read, topic_name_2 ,@callback_2);
% node_2 = ros2subscriber(sensors_read, topic_name_2, @callback_2);

% % Get the ROS 2 node's PID safely
% % [status_pid_1, result_1] = system('pgrep talker_1');
% % [status_pid_2, result_2] = system('pgrep talker_2');
[status_pid_1, result_1] = system('pgrep rbobot_state_pub');
[status_pid_2, result_2] = system('pgrep ros2_control_no');
[status_pid_3, result_3] = system('pgrep aurora_talker');
% 
subs_pid_1 = str2double(strtrim(result_1));
subs_pid_2 = str2double(strtrim(result_2));
subs_pid_3 = str2double(strtrim(result_3));
% 
% Let the node run for 20 s
pause(20);
% 
% Kill command
% kill_command_1 = sprintf('kill -9 %d', node_pid_1);
% kill_command_2 = sprintf('kill -9 %d', node_pid_2);
kill_command_1 = sprintf('kill -SIGINT %d', subs_pid_1);
kill_command_2 = sprintf('kill -SIGINT %d', subs_pid_2);
kill_command_3 = sprintf('kill -SIGINT %d', subs_pid_3);
[status_kill_1, cmdout_1] = system(kill_command_1);
[status_kill_2, cmdout_2] = system(kill_command_2);
[status_kill_3, cmdout_3] = system(kill_command_3);

% Remove the ROS 2 node from the network
clear("sensors_read");

% Plot Aurora orientation
figure;
plot(data_2(:,2));
hold on;
plot(data_2(:,3));
plot(data_2(:,4));
plot(data_2(:,5));
title("Aurora Sensor - Orientation")
legend("Orientation.w", "Orientation.X", "Orientation.Y", "Orientation.Z");

% Plot Aurora position
figure;
plot(data_2(:,6));
hold on;
plot(data_2(:,7));
plot(data_2(:,8));
title("Aurora Sensor - Position")
legend("X","Y","Z");

% Plot force data
figure; 
plot(data_1(:,2));
hold on;
plot(data_1(:,3));
plot(data_1(:,4));
title("ATI")
legend("F_x", "F_y", "F_z");

% Write data into a csv file
writematrix(data_1, "ft_data.csv");
writematrix(data_2, "aurora_transform.csv");
% % writecell(data_2, "hello_BRL_txt.csv");
% 
function callback_1(msg)
    global data_1;
    % timestamp = datetime('now', 'Format', 'yyyy-MM-dd HH:mm:ss.SSS'); % Get current system time
    % seconds = posixtime(timestamp); % Convert datetime to POSIX time (seconds since epoch)
    % % Get nanoseconds part (milliseconds * 1e6 for nanoseconds)
    % nanoseconds = mod(seconds, 1) * 1e9;
    timestamp_sec = msg.header.stamp.sec;
    timestamp_nsec = msg.header.stamp.nanosec;
    full_timestamp = double(timestamp_sec) + double(timestamp_nsec);
    % % Combine seconds and nanoseconds into a floating-point number
    % full_timestamp = floor(seconds) + nanoseconds / 1e9;
    force_x = msg.wrench.force.x;
    force_y = msg.wrench.force.y;
    force_z = msg.wrench.force.z;
    torque_x = msg.wrench.torque.x;
    torque_y = msg.wrench.torque.y;
    torque_z = msg.wrench.torque.z;
    data_1 = [data_1; [full_timestamp, force_x, force_y, force_z, torque_x, torque_y, torque_z]];
end
% 
function callback_2(msg)

    global data_2;
    
    % Threshold to remove data outliers
    threshold = 1e10;
    % Get generate timestamp
    timestamp = datetime('now', 'Format', 'yyyy-MM-dd HH:mm:ss.SSS'); % Get current system time
    seconds = posixtime(timestamp); % Convert datetime to POSIX time (seconds since epoch)
    % Get nanoseconds part (milliseconds * 1e6 for nanoseconds)
    nanoseconds = mod(seconds, 1) * 1e9;
    % Combine seconds and nanoseconds into a floating-point number
    full_timestamp = floor(seconds) + nanoseconds / 1e9;
    transform_data_raw = msg.data;
    transform_data = transform_data_raw(abs(transform_data_raw) < threshold);
    disp(transform_data);
    % quat = [transform_data_raw(1), transform_data_raw(2), transform_data_raw(3), transform_data_raw(4)];
    % euler_angle = quat2eul(quat, 'ZYX');
    % transform_data = [euler_angle, transform_data_raw(5), transform_data_raw(6), transform_data_raw(7)];
    % data_2 = [data_2; [full_timestamp, transform_data_raw]];
    data_2 = [data_2; [full_timestamp, transform_data']];
    % disp(transform_data);
end
