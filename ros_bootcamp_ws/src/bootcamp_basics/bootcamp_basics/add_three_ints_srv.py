import rclpy
from rclpy.node import Node
from bootcamp_interfaces.srv import AddThreeInts


class AddThreeIntsServer(Node):
    def __init__(self):
        super().__init__('add_three_ints_server')
        self.srv = self.create_service(AddThreeInts, 'add_three_ints', self.callback)
        self.get_logger().info("Service /add_three_ints ready (bootcamp_interfaces/AddThreeInts)")

    def callback(self, request: AddThreeInts.Request, response: AddThreeInts.Response):
        response.sum = request.a + request.b + request.c
        self.get_logger().info(f"request: a={request.a} b={request.b} c={request.c} -> sum={response.sum}")
        return response

def main():
    rclpy.init()
    node = AddThreeIntsServer()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
