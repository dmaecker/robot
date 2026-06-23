from setuptools import find_packages, setup

package_name = 'robot_web_control'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', ['launch/web_control.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Dennis',
    maintainer_email='dennis@example.com',
    description='Web UI hosting a live MJPEG webcam stream and a joystick that publishes /cmd_vel',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'web_node = robot_web_control.web_node:main',
        ],
    },
)
