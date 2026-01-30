#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$SCRIPT_DIR/.."
HUMBLE_WS_DIR="$BASE_DIR/humble_ws"
UNDERLAY_WS_DIR="$BASE_DIR/underlay_ws"

set -ex # exit on error, show commands

echo "=========================================="
echo "Building underlay workspace"
echo "=========================================="

# Check that humble_ws is built
if [ ! -d "$HUMBLE_WS_DIR/install" ]; then
  echo "Error: humble_ws/install directory not found."
  echo "Please build humble_ws first with: make humble"
  exit 1
fi

# Create underlay workspace directory structure if needed
mkdir -p "$UNDERLAY_WS_DIR/src"
cd "$UNDERLAY_WS_DIR" || exit

# Import repositories from .repos file if src is empty
if [ ! -d "$UNDERLAY_WS_DIR/src/vision_opencv" ]; then
  echo "Importing repositories from underlay.repos..."
  if [ -f "$UNDERLAY_WS_DIR/underlay.repos" ]; then
    vcs import src < underlay.repos
  else
    echo "Error: underlay.repos file not found in $UNDERLAY_WS_DIR"
    exit 1
  fi
fi

# Source humble_ws before building
echo "Sourcing humble_ws setup..."
source "$HUMBLE_WS_DIR/install/setup.bash"

# Build the workspace
echo "Building workspace in: $(pwd)"
colcon build --symlink-install

echo "=========================================="
echo "Underlay workspace build complete!"
echo "Source with: source underlay_ws/install/setup.bash"
echo "=========================================="
