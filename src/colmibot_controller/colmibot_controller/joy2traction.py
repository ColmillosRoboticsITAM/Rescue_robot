import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy
from std_msgs.msg import Float32MultiArray


class JoyToTraccion(Node):

    def __init__(self):
        super().__init__('joy2traction')

        self.publisher = self.create_publisher(
            Float32MultiArray,
            '/traction',
            10
        )

        self.subscription = self.create_subscription(
            Joy,
            '/joy',
            self.joy_callback,
            10
        )

    def joy_callback(self, msg):

        # Inicializar los 6 valores en 0
        data = [0.0] * 6

        # Ejes
        eje1 = msg.axes[1]
        eje3 = msg.axes[3]
        eje4 = msg.axes[4]

        # Botones
        R1 = msg.buttons[5]

        # Gatillos (analógicos PS4)
        L2 = msg.axes[2]
        R2 = msg.axes[5]

        # L2 presionado
        if R2 < -0.3:
            data[0] = -eje4/0.7
            data[1] = -eje1/0.7

        # R2 presionado
        if L2 < -0.3:
            data[2] = eje4/0.7
            data[3] = eje1/0.7

        # R1 presionado
        if R1 == 1:
            data[4] = -eje1
            data[5] = -eje3/0.55

        msg_out = Float32MultiArray()
        msg_out.data = data

        self.publisher.publish(msg_out)

        # Debug
        print(data)


def main():
    rclpy.init()
    node = JoyToTraccion()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()