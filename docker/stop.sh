#!/bin/bash
# Stop and remove the ROS Humble container

cd "$(dirname "$0")"

echo "Stopping ROS Humble container..."
docker compose down

echo "Container stopped."
