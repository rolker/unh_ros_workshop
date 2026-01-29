# ROS Bootcamp Workspace

## Build
```bash
source /opt/ros/humble/setup.bash
cd ~/ros_bootcamp_ws
colcon build
source install/setup.bash
```

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
