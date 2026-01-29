#!/bin/bash
# Test script to verify Docker setup is working correctly

echo "======================================"
echo "ROS Humble Docker Setup - Test Suite"
echo "======================================"
echo ""

cd "$(dirname "$0")"

# Check if container is running
echo "1. Checking if container is running..."
if docker ps | grep -q ros-humble-dev; then
    echo "   ✓ Container is running"
else
    echo "   ✗ Container is not running. Starting it..."
    ./start.sh
    sleep 3
fi
echo ""

# Test GPU access
echo "2. Testing GPU access..."
if docker exec ros-humble-dev nvidia-smi &>/dev/null; then
    echo "   ✓ GPU is accessible"
    docker exec ros-humble-dev nvidia-smi --query-gpu=name,driver_version,cuda_version --format=csv,noheader
else
    echo "   ✗ GPU not accessible"
fi
echo ""

# Test ROS Humble
echo "3. Testing ROS Humble installation..."
if docker exec ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && ros2 topic list" &>/dev/null; then
    ROS_VERSION=$(docker exec ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && echo \$ROS_DISTRO")
    echo "   ✓ ROS $ROS_VERSION is working"
else
    echo "   ✗ ROS Humble not working"
fi
echo ""

# Test workspace mounting
echo "4. Testing workspace volume mounting..."
PKG_COUNT=$(docker exec ros-humble-dev bash -c "ls /workspace/ros_bootcamp_ws/src | wc -l")
if [ "$PKG_COUNT" -gt 0 ]; then
    echo "   ✓ Workspace mounted ($PKG_COUNT packages found)"
    docker exec ros-humble-dev bash -c "ls /workspace/ros_bootcamp_ws/src"
else
    echo "   ✗ Workspace not properly mounted"
fi
echo ""

# Test if packages are built
echo "5. Testing ROS workspace build..."
if docker exec ros-humble-dev bash -c "[ -d /workspace/ros_bootcamp_ws/install ]" &>/dev/null; then
    BUILT_PKGS=$(docker exec ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && source /workspace/ros_bootcamp_ws/install/setup.bash && ros2 pkg list | grep bootcamp | wc -l" 2>/dev/null)
    if [ "$BUILT_PKGS" -gt 0 ]; then
        echo "   ✓ Workspace is built ($BUILT_PKGS bootcamp packages)"
    else
        echo "   ⚠ Workspace exists but packages not found. Run ./build.sh"
    fi
else
    echo "   ⚠ Workspace not built yet. Run ./build.sh"
fi
echo ""

# Test host ROS version
echo "6. Verifying host ROS isolation..."
HOST_ROS=$(printenv ROS_DISTRO 2>/dev/null || echo "none")
CONTAINER_ROS=$(docker exec ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && echo \$ROS_DISTRO" 2>/dev/null)
if [ "$HOST_ROS" != "$CONTAINER_ROS" ]; then
    echo "   ✓ Host ($HOST_ROS) and container ($CONTAINER_ROS) are properly isolated"
else
    echo "   ⚠ Warning: Host and container have same ROS version"
fi
echo ""

echo "======================================"
echo "Test suite complete!"
echo "======================================"
