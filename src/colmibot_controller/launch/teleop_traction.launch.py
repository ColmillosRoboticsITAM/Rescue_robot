from launch import LaunchDescription
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    pkg_dir = get_package_share_directory("colmibot_controller")

    joy_config = os.path.join(pkg_dir, "config", "joy_config.yaml")

    joy_node = Node(
        package="joy",
        executable="joy_node",
        name="joy_node",
        parameters=[joy_config]
    )

    joy2traction = Node(
        package="colmibot_controller",
        executable="joy2traction",
        name="joy2traction",
        output="screen"
    )

    """ serial_entropy = Node(
        package="colmibot_controller",
        executable="serial_entropy",
        name="serial_entropy",
        parameters=[
            {"port": "/dev/ttyUSB0"},
            {"baudrate": 115200}
        ],
        output="screen"
    ) """

    return LaunchDescription([
        joy_node,
        joy2traction,
        #serial_entropy
    ])