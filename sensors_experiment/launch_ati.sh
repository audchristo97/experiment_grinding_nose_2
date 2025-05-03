#!/bin/bash

echo "Sourcing to workspace ROS 2 setup.bash file..."
source ~/experiment_grinding_nose/sensors_experiment/install/setup.bash

echo "Launching nodes..."
ros2 launch net_ft_driver net_ft_broadcaster.launch.py ip_address:=192.168.1.2 sensor_type:=ati_axia rdt_sampling_rate:=1000 use_hardware_biasing:=true