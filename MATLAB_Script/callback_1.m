% Callback functions for the ROS 2 Subscribers
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

