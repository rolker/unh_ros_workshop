#!/bin/bash
set -e

# Source ROS Humble
source /opt/ros/humble/setup.bash

# Source workspace if built
if [ -f /workspace/ros_bootcamp_ws/install/setup.bash ]; then
    source /workspace/ros_bootcamp_ws/install/setup.bash
fi

# Execute the command passed to docker run
exec "$@"
