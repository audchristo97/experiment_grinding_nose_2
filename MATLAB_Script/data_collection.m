global data_1;
global data_2;

ft_data = data_1;
aurora_data = data_2;

% Save data to CSV file

base_name_aurora = 'aurora_transform';
base_name_ft = 'ati_ft_data';
ext = '.csv';
count = 1;
while exist([base_name_aurora '_' num2str(count) ext], 'file')
    count = count + 1;
end

while exist([base_name_ft '_' num2str(count) ext], 'file')
    count = count + 1;
end

aurora_file = [base_name_aurora '_' num2str(count) ext];
header_aurora = "Time, Orientation_W, Orientation_X, Orientation_Y, Orientation_Z, Position_X, Position_Y, Position_Z";
writelines(header_aurora, aurora_file);

ati_file = [base_name_ft '_' num2str(count) ext];
header_ati = "Time, F_X, F_Y, F_Z, T_X, T_Y, T_Z";
writelines(header_ati, ati_file);

writematrix(aurora_data, aurora_file, 'WriteMode','overwrite');
writematrix(ft_data, ati_file,'WriteMode','overwrite');