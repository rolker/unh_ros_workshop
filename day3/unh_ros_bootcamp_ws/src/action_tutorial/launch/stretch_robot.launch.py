import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node

def generate_launch_description():
    # Launch arguments
    use_sim_time_param = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation/Gazebo clock'
    )
    
    teleop_type_param = DeclareLaunchArgument(
        'teleop_type',
        default_value='joystick',
        description="how to teleop ('keyboard', 'joystick' or 'none')"
    )
    
    rviz_param = DeclareLaunchArgument(
        'use_rviz',
        default_value='True',
        choices=['True', 'False'],
        description='Whether to start RViz'
    )

    # Package paths
    stretch_core_path = get_package_share_directory('stretch_core')
    stretch_navigation_path = get_package_share_directory('stretch_nav2')
    action_tut_path = get_package_share_directory('action_tutorial')
    realsense2_camera_path = get_package_share_directory('realsense2_camera')

    # Include launch files
    # mode choices=['position', 'navigation', 'trajectory', 'gamepad']
    stretch_driver_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(stretch_core_path, 'launch', 'stretch_driver.launch.py')
        ),
        launch_arguments={'mode': 'navigation', 'broadcast_odom_tf': 'True'}.items(),
        condition=UnlessCondition(LaunchConfiguration('use_sim_time'))
    )
    # ## navigation for cmd_vel

    rplidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(stretch_core_path, 'launch', 'rplidar.launch.py')
        ),
        condition=UnlessCondition(LaunchConfiguration('use_sim_time'))
    )

    realsense_launch = IncludeLaunchDescription(
    PythonLaunchDescriptionSource(
        os.path.join(realsense2_camera_path, 'launch', 'rs_launch.py'))
    # ),
    #     launch_arguments={
    #         'serial_no': "'130322272495'",   # <-- your camera serial
    #     }.items()
    )

    base_teleop_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(stretch_navigation_path, 'launch', 'teleop_twist.launch.py')
        ),
        launch_arguments={'teleop_type': LaunchConfiguration('teleop_type')}.items()
    )
    

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', os.path.join(action_tut_path, 'config', 'unh_bootcamp_config.rviz')],
        condition=IfCondition(LaunchConfiguration('use_rviz'))
    )

    # Return launch description
    return LaunchDescription([
        teleop_type_param,
        use_sim_time_param,
        rviz_param,
        stretch_driver_launch,
        rplidar_launch,
        base_teleop_launch,
        realsense_launch,
        rviz_node,
    ])
