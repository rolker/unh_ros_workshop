#!/bin/bash
# Build ROS workspace inside the Humble container

cd "$(dirname "$0")"

if ! docker ps | grep -q ros-humble-dev; then
    echo "Container is not running. Starting it now..."
    ./start.sh
    sleep 2
fi

echo "Building ROS workspace in Humble container..."
docker exec -it ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && cd /workspace/ros_bootcamp_ws && colcon build --symlink-install"
