from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
import os

def generate_launch_description():
    # Get the path to the installed models directory
    pkg_share = FindPackageShare('bootcamp_vision_cpp')
    yolo_model_path = PathJoinSubstitution([pkg_share, 'models', 'tinyyolov2.onnx'])
    face_det_model_path = PathJoinSubstitution([pkg_share, 'models', 'face_detector.caffemodel'])
    face_rec_model_path = PathJoinSubstitution([pkg_share, 'models', 'w600k_r50.onnx'])
    
    enable_faces_arg = DeclareLaunchArgument(
        'enable_faces',
        default_value='True',
        description='Enable face detection and enrollment services'
    )
    
    return LaunchDescription([
        enable_faces_arg,
        
        Node(
            package='bootcamp_vision_cpp',
            executable='camera_pub_node',
            name='camera_publisher',
            parameters=[{
                'cam_id': 0,
                'fps': 30.0,
            }],
            output='screen'
        ),
        Node(
            package='bootcamp_vision_cpp',
            executable='vision_node',
            name='vision_node',
            parameters=[{
                'yolo_model': yolo_model_path,
                'yolo_conf': 0.35,
                'face_rec_model': face_rec_model_path,
                'face_thresh': 0.45,
                'enable_face_detection': LaunchConfiguration('enable_faces'),
            }],
            output='screen'
        ),
    ])


