#!/bin/bash

echo "Sourcing to workspace ROS 2 setup.bash file..."
source ~/experiment_grinding_nose/sensors_experiment/install/setup.bash

echo "Running Aurora ROS 2 node"
ros2 run aurora_ndi_pkg aurora_talker