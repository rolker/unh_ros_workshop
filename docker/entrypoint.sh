#!/bin/bash
set -e

# Source ROS Humble
source /opt/ros/humble/setup.bash

# Install dependencies if workspace source exists and AUTO_ROSDEP=1
if [ "$AUTO_ROSDEP" = "1" ] && [ -d /workspace/ros_bootcamp_ws/src ]; then
    echo "Installing workspace dependencies with rosdep..."
    cd /workspace/ros_bootcamp_ws
    rosdep update || true
    rosdep install --from-paths src --ignore-src -y || echo "Some dependencies could not be installed"
fi

# Source workspace if built
if [ -f /workspace/ros_bootcamp_ws/install/setup.bash ]; then
    source /workspace/ros_bootcamp_ws/install/setup.bash
fi

# Execute the command passed to docker run
exec "$@"
