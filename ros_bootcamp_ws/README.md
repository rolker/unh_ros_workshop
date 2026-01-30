# ROS Bootcamp Workspace

This workspace contains the bootcamp example code for learning ROS2 concepts.

## Build

```bash
# From repository root
make bootcamp

# Or manually
source humble_ws/install/setup.bash
source underlay_ws/install/setup.bash
cd ros_bootcamp_ws
colcon build --symlink-install
```

## Setup

After building, source only the bootcamp workspace:

```bash
source ros_bootcamp_ws/install/setup.bash
```

This automatically includes the underlay and humble workspaces.

## Examples

## Examples

## Basics: pub/sub
Terminal A:
```bash
ros2 run bootcamp_basics talker
```
Terminal B:
```bash
ros2 run bootcamp_basics listener
```

## Basics: service
Terminal A:
```bash
ros2 run bootcamp_basics add_two_ints_srv
```
Terminal B:
```bash
ros2 run bootcamp_basics add_two_ints_client 7 35
```

## Vision over topics
Terminal A:
```bash
ros2 run bootcamp_vision camera_pub
```

Terminal B:
```bash
ros2 run bootcamp_vision vision_node
```

View:
```bash
rqt_image_view
ros2 topic echo /vision/detections
```

## Turtlesim
Terminal A:
```bash
ros2 run turtlesim turtlesim_node
```
Terminal B:
```bash
ros2 run bootcamp_basics turtlesim_square
```
