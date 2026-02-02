# ROS 2 Turtlesim Action Tutorial (Bootcamp)

## Goal

The goal of this tutorial is to demonstrate how to use **ROS 2 actions** with **turtlesim** by implementing a custom action server and client.

This tutorial is based on:

* ROS 2 Humble Hawksbill
* `turtlesim` from [ros_tutorials](https://github.com/ros/ros_tutorials)

---

## Install and Run Turtlesim

```bash
sudo apt install ros-humble-turtlesim
```

Verify the package:

```bash
ros2 pkg executables turtlesim
```

Run turtlesim:

```bash
ros2 run turtlesim turtlesim_node
```

---

## Turtlesim Interfaces

### Topics

```bash
ros2 topic list
```

Example output:

```
/parameter_events
/rosout
/topic_statistics
/turtle1/cmd_vel
/turtle1/color_sensor
/turtle1/pose
```

### Actions

```bash
ros2 action list
```

Example output:

```
/turtle1/rotate_absolute
```

### Services

```bash
ros2 service list
```

Example output:

```
/clear
/kill
/reset
/spawn
/turtle1/set_pen
/turtle1/teleport_absolute
/turtle1/teleport_relative
```

---

## Action Design

We will implement:

* **FollowFullTrajectory** – send an entire trajectory at once

We need a custom action definition, so we first create an interface package.

---

## Create Interface Package

```bash
mkdir -p ~/unh_ros_bootcamp_ws/src
cd ~/unh_ros_bootcamp_ws/src
ros2 pkg create unh_bc_interfaces
cd unh_bc_interfaces
mkdir action
```

---

### FollowFullTrajectory.action

Create this file in `action/FollowFullTrajectory.action`:

```text
# Goal
geometry_msgs/Point[] waypoints
---
# Result
bool success
---
# Feedback
float32 distance_remaining
```

---

## Update `CMakeLists.txt` and `package.xml`

### CMakeLists.txt

```cmake
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/FollowFullTrajectory.action"
  DEPENDENCIES geometry_msgs
)
```

### package.xml

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<depend>geometry_msgs</depend>
<depend>action_msgs</depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

---

## Build Interface Package

```bash
cd ~/unh_ros_bootcamp_ws
colcon build --symlink-install
source install/setup.bash
```

Verify the action interface:

```bash
ros2 interface show unh_bc_interfaces/action/FollowFullTrajectory
```

---

## Create Action Server and Client Package

```bash
cd ~/unh_ros_bootcamp_ws/src
ros2 pkg create --build-type ament_python action_tutorial
```

Update `package.xml` to include the interface package:

```xml
<depend>unh_bc_interfaces</depend>
```

---

### Action Server: `follow_full_trajectory_server.py`

Create `action_tutorial/action_tutorial/follow_full_trajectory_server.py`.

Reset turtlesim:

```bash
ros2 service call /reset std_srvs/srv/Empty "{}"
```

Send a goal:

```bash
ros2 action send_goal /follow_full_trajectory \
unh_bc_interfaces/action/FollowFullTrajectory \
"{waypoints: [{x: 1.0, y: 1.0, z: 0.0}, {x: 2.0, y: 1.0, z: 0.0}]}"
```

---

### Action Client: `follow_full_trajectory_client.py`

Create `action_tutorial/action_tutorial/follow_full_trajectory_client.py`.

This client will send goals to the server and handle feedback/results.

---

## ROS 2 Action Cheat Sheet

```bash
ros2 action list
ros2 action list -t
ros2 action info /action_name
ros2 interface show <type>
ros2 action send_goal /action_name <type> "{data}" --feedback
```

---

## Environment Setup

```bash
# Source ROS
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
source ~/.bashrc

# Optional: ROS domain and localhost-only settings
export ROS_DOMAIN_ID=<your_domain_id>
export ROS_LOCALHOST_ONLY=1
echo "export ROS_LOCALHOST_ONLY=1" >> ~/.bashrc
```