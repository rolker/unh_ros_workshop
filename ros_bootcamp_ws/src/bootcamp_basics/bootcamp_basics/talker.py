import rclpy
from rclpy.node import Node
from std_msgs.msg import String

class Talker(Node):
    def __init__(self):
        super().__init__('talker')
        self.pub = self.create_publisher(String, 'chatter', 10)
        self.timer = self.create_timer(0.5, self.on_timer)
        self.count = 0
        self.get_logger().info("Talker started. Publishing on /chatter")

    def on_timer(self):
        msg = String()
        msg.data = f"hello {self.count}"
        self.get_logger().info(f"publishing : {msg.data}")

        self.pub.publish(msg)
        self.count += 1

def main():
    rclpy.init()
    node = Talker()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
