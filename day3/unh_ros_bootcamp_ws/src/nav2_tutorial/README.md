# Simulation 

ros2 launch nav2_tutorial turtlebot_nav2.launch.py params_file:=<path to yaml>


### Switching Controllers
We provide a helper script, switch_Controller_config.py, which exposes a service to toggle controllers. When set to true, it switches to MPPI and adjusts yaw_goal_tolerance accordingly.


Use the `/switch_controller` service to toggle between MPPI and DWB:
- **Switch to MPPI (no heading):**

  ```bash
  ros2 service call /switch_controller std_srvs/srv/SetBool "{data: true}"
  ```

  Response:\
  `Switched to MPPIController; set yaw_goal_tolerance = 3.145`

- **Switch to DWB:**

  ```bash
  ros2 service call /switch_controller std_srvs/srv/SetBool "{data: false}"
  ```

  Response:\
  `Switched to DWBController; set yaw_goal_tolerance = 0.08`

### Verifying `yaw_goal_tolerance`

To confirm that the `yaw_goal_tolerance` value has been updated:

```bash
ros2 param get /controller_server general_goal_checker.yaw_goal_tolerance
```


---

## Teleop the Robot
By default teleoperating with keyboard is enabled. You can run 

```bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r /cmd_vel:=/stretch/cmd_vel
```
Make sure you are aware of the speed it is at before moving. If you already moved and wanted to reduce the speed be aware that it will apply the last command you applied so if you don't want the robot to move hit K(button that stops the robot) before reducing the speed.

You can also use the gamepad controller

first switch modes with 

```bash
 ros2 service call /switch_to_gamepad_mode std_srvs/srv/Trigger
```
then run 

```bash
ros2 run stretch_core remote_gamepad
```
<!-- The launch files expose the launch argument "teleop_type". By default, this argument is set to "joystick", which launches joystick teleop in the terminal with the xbox controller that ships with Stretch RE1. The xbox controller utilizes a dead man's switch safety feature to avoid unintended movement of the robot. This is the switch located on the front left side of the controller marked "LB". Keep this switch pressed and translate or rotate the base using the joystick located on the right side of the xbox controller.

If the xbox controller is not available, the following commands will launch mapping and navigation, respectively, with keyboard teleop:

```bash
ros2 launch stretch_nav2 offline_mapping.launch.py teleop_type:=keyboard
```
or
```bash
ros2 launch stretch_nav2 navigation.launch.py teleop_type:=keyboard map:=${HELLO_FLEET_PATH}/maps/<map_name>.yaml
```
--> 

# REAL robot

https://docs.hello-robot.com/0.2/stretch-tutorials/ros2/navigation_stack/

the first step is to map the space that the robot will navigate in
ros2 launch stretch_nav2 offline_mapping.launch.py
ros2 launch stretch_nav2 offline_mapping.launch.py teleop_type:=keyboard


save map
ros2 run nav2_map_server map_saver_cli -f ${HELLO_FLEET_PATH}/maps/<map_name>

ros2 launch stretch_nav2 navigation.launch.py map:=${HELLO_FLEET_PATH}/maps/<map_name>.yaml
