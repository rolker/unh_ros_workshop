#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HUMBLE_WS_DIR="$SCRIPT_DIR/../humble_ws"

set -ex # exit on error, show commands

echo "=========================================="
echo "Building ROS2 Humble workspace"
echo "=========================================="

if [ ! -d "$HUMBLE_WS_DIR/src" ]; then
  echo "Error: humble_ws/src directory not found."
  echo "Please run initialize_humble_workspace.bash first."
  exit 1
fi

cd "$HUMBLE_WS_DIR" || exit

echo "Building workspace in: $(pwd)"
colcon build --symlink-install

echo "=========================================="
echo "Humble workspace build complete!"
echo "Source with: source humble_ws/install/setup.bash"
echo "=========================================="

