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
data_2 = [data_2; [full_timestamp, transform_data(1), transform_data(2), transform_data(3), transform_data(4), transform_data(5), transform_data(6), transform_data(7)]];
% disp(transform_data);
end