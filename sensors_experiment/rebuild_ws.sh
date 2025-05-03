#!/bin/bash

echo "Clean workspace"
rm -rf build/ log/ install/

echo "Sourcing to Main ROS 2 setup bash"
source /opt/ros/humble/setup.bash

echo "Build workspace..."
colcon build --symlink-install