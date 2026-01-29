import sys
import rclpy
from rclpy.node import Node
from bootcamp_interfaces.srv import AddThreeInts


class AddThreeIntsClient(Node):
    def __init__(self):
        super().__init__('add_three_ints_client')
        self.cli = self.create_client(AddThreeInts, 'add_three_ints')
        while not self.cli.wait_for_service(timeout_sec=1.0):
            self.get_logger().info("waiting for /add_three_ints service...")

    def call(self, a: int, b: int, c: int):
        req = AddThreeInts.Request()
        req.a = int(a)
        req.b = int(b)
        req.c = int(c)
        future = self.cli.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        return future.result()

def main():
    if len(sys.argv) != 4:
        print("Usage: ros2 run bootcamp_basics add_three_ints_client <a> <b> <c>")
        return
    a, b ,c  = sys.argv[1], sys.argv[2], sys.argv[3]

    rclpy.init()
    node = AddThreeIntsClient()
    resp = node.call(a, b, c)
    node.get_logger().info(f"result: {a} + {b} + {c} = {resp.sum}")
    node.destroy_node()
    rclpy.shutdown()
