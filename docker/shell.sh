#!/bin/bash
# Open an interactive shell in the running ROS Humble container

cd "$(dirname "$0")"

if ! docker ps | grep -q ros-humble-dev; then
    echo "Container is not running. Starting it now..."
    ./start.sh
    sleep 2
fi

echo "Opening shell in ROS Humble container..."
docker exec -it ros-humble-dev bash
