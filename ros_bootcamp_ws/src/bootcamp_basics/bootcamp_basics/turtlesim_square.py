import math
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

class TurtleSquare(Node):
    def __init__(self):
        super().__init__('turtlesim_square')
        self.pub = self.create_publisher(Twist, '/turtle1/cmd_vel', 10)
        self.timer = self.create_timer(0.1, self.step)
        self.state = "forward"
        self.t = 0.0
        self.side_time = 2.0   # tune for your speed
        self.turn_time = 1.6   # tune for ~90deg
        self.get_logger().info("Publishing /turtle1/cmd_vel to draw a square.")

    def step(self):
        msg = Twist()
        self.t += 0.1

        if self.state == "forward":
            msg.linear.x = 1.5
            if self.t >= self.side_time:
                self.state = "turn"
                self.t = 0.0
        else:
            msg.angular.z = 1.0
            if self.t >= self.turn_time:
                self.state = "forward"
                self.t = 0.0

        self.pub.publish(msg)

def main():
    rclpy.init()
    node = TurtleSquare()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
