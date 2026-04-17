import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import serial


class SerialBridge(Node):

    def __init__(self):
        super().__init__('serial_bridge')

        self.ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

        # Suscriptor a cmd_vel
        self.subscription = self.create_subscription(
            Twist,
            '/colmibot/cmd_vel',
            self.cmd_callback,
            10
        )

    def cmd_callback(self, msg):
        v = msg.linear.x
        w = msg.angular.z

        # Formato simple: "v,w\n"
        data = f"{v:.2f},{w:.2f}\n"

        self.ser.write(data.encode())

        self.get_logger().info(f"Sent: {data.strip()}")


def main():
    rclpy.init()
    node = SerialBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()