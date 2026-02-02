import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

from launch.actions import LogInfo


def generate_launch_description():
    # --- Simulator Launch ---
    pkg_share = get_package_share_directory("nav2_tutorial")
    nav2_params = os.path.join(pkg_share, "config", "jazzy_nav2_params.yaml")

    tb3_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("nav2_bringup"),
                "launch",
                "tb3_simulation_launch.py",
            )
        ),
        launch_arguments={
            "params_file": nav2_params,
            "headless": "False"}.items(),
    )

    # waits for subscriber
    # ros2 topic pub --once -w 1 /charging std_msgs/msg/Bool "{data: True}"
    # --qos-durability transient_local --qos-reliability reliable
    # charging_pub = ExecuteProcess(
    #     cmd=[
    #         "ros2",
    #         "topic",
    #         "pub",
    #         "--once",
    #         "-w",
    #         "1",
    #         "/charging",
    #         "std_msgs/msg/Bool",
    #         "{data: True}",
    #         "--qos-durability",
    #         "transient_local",
    #         "--qos-reliability",
    #         "reliable",
    #     ],
    #     output="screen",
    # )

    # --- Return one LaunchDescription with everything included ---
    return LaunchDescription(
        [   tb3_launch,
        ]
    )
