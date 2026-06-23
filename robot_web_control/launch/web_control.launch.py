from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    video_device = LaunchConfiguration('video_device')
    width = LaunchConfiguration('width')
    height = LaunchConfiguration('height')
    port = LaunchConfiguration('port')

    return LaunchDescription([
        DeclareLaunchArgument('video_device', default_value='/dev/video0'),
        DeclareLaunchArgument('width', default_value='640'),
        DeclareLaunchArgument('height', default_value='480'),
        DeclareLaunchArgument('port', default_value='8080'),

        # Camera in MJPG mode: publishes /image_raw/compressed without CPU re-encode
        Node(
            package='v4l2_camera',
            executable='v4l2_camera_node',
            name='camera',
            parameters=[{
                'video_device': video_device,
                'pixel_format': 'MJPG',
                'image_size': [width, height],
            }],
            output='screen',
        ),

        Node(
            package='robot_web_control',
            executable='web_node',
            name='web_node',
            parameters=[{
                'port': port,
                'image_topic': '/image_raw/compressed',
                'cmd_topic': '/cmd_vel',
                'max_linear': 0.3,
                'max_angular': 1.5,
                'stream_fps': 15.0,
            }],
            output='screen',
        ),
    ])
