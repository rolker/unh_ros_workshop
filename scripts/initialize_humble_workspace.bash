#!/bin/bash

set -ex # exit on error, show commands

echo "=========================================="
echo "Initializing ROS2 Humble workspace"
echo "=========================================="

# from https://docs.ros.org/en/humble/Installation/Alternatives/Ubuntu-Development-Setup.html

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HUMBLE_WS_DIR="$SCRIPT_DIR/../humble_ws"

# Skip apt installs if already done (check for a marker package)
if ! dpkg -l | grep -q ros-dev-tools; then
  echo "Installing development tools..."
  sudo apt update && sudo apt install -y \
    python3-flake8-docstrings \
    python3-pip \
    python3-pytest-cov \
    ros-dev-tools

  sudo apt install -y \
     python3-flake8-blind-except \
     python3-flake8-builtins \
     python3-flake8-class-newline \
     python3-flake8-comprehensions \
     python3-flake8-deprecated \
     python3-flake8-import-order \
     python3-flake8-quotes \
     python3-pytest-repeat \
     python3-pytest-rerunfailures
else
  echo "Development tools already installed, skipping apt install"
fi

echo "Creating workspace directory: $HUMBLE_WS_DIR"
mkdir -p "$HUMBLE_WS_DIR/src"
cd "$HUMBLE_WS_DIR" || exit

# Only import if src is empty
if [ -z "$(ls -A src 2>/dev/null)" ]; then
  echo "Importing ROS2 Humble repositories..."
  vcs import --input https://raw.githubusercontent.com/ros2/ros2/humble/ros2.repos src
else
  echo "Source directory not empty, skipping vcs import"
fi

# Initialize rosdep only if needed
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  echo "Initializing rosdep..."
  sudo rosdep init
else
  echo "rosdep already initialized, skipping"
fi

echo "Updating rosdep..."
rosdep update

echo "Checking dependencies..."
rosdep check --from-paths src --ignore-src || echo "Some dependencies missing (expected on unsupported platform)"

echo "=========================================="
echo "Humble workspace initialized!"
echo "Run 'make humble' or scripts/build_humble_ws.bash to build"
echo "=========================================="
