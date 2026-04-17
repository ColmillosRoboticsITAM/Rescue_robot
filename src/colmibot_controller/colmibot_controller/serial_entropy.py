import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray
import serial


class SerialBridgeEntropy(Node):

    def __init__(self):
        super().__init__('serial_entropy')

        self.ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

        self.subscription = self.create_subscription(
            Float32MultiArray,
            '/traction',
            self.callback,
            10
        )

    def callback(self, msg):

        # convertir lista a string CSV
        data = ",".join([f"{x:.2f}" for x in msg.data]) + "\n"

        self.ser.write(data.encode())

        print("Sent:", data.strip())


def main():
    rclpy.init()
    node = SerialBridgeEntropy()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()