#!/bin/bash
# Start ROS Humble container with Docker Compose

cd "$(dirname "$0")"

# Get host user UID/GID
export USER_UID=$(id -u)
export USER_GID=$(id -g)

# Allow X11 connections
xhost +local:docker 2>/dev/null || echo "Note: xhost not available, GUI apps may not work"

echo "Starting ROS Humble container..."
docker compose up -d

echo ""
echo "Container started! Use './shell.sh' to access the container."
echo "Use './stop.sh' to stop the container."
