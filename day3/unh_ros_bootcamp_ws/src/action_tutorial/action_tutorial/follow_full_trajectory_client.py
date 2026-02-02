import rclpy
from rclpy.node import Node
from turtlesim.srv import SetPen
from unh_bc_interfaces.action import FollowFullTrajectory
from rclpy.action import ActionClient
import time
from geometry_msgs.msg import Point
from action_msgs.msg import GoalStatus

# Linear Scripting
class TrajectoryClient(Node):
    def __init__(self):
        super().__init__('trajectory_client')
        self.full_client = ActionClient(self, FollowFullTrajectory, 'follow_full_trajectory')

        self.pen_client = self.create_client(SetPen, '/turtle1/set_pen')

    def set_pen(self, r, g, b):
        while not self.pen_client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info("Waiting for /turtle1/set_pen...")
        req = SetPen.Request()
        req.r = r
        req.g = g
        req.b = b
        req.width = 3
        req.off = False
        self.pen_client.call_async(req)


    def feedback_callback(self, feedback_msg):
        distance = feedback_msg.feedback.distance_remaining
        print(f"Distance remaining: {distance:.2f}")
        self.get_logger().info(f"Distance remaining: {distance:.2f}")


    def send_full_trajectory(self, trajectory):
        self.set_pen(0, 0, 255)  # blue

        goal_msg = FollowFullTrajectory.Goal()

        points = []
        for x, y in trajectory:
            p = Point()
            p.x = float(x)
            p.y = float(y)
            p.z = 0.0
            points.append(p)

        goal_msg.waypoints = points

        # Wait for server
        self.get_logger().info("Waiting for follow_full_trajectory action server...")
        if not self.full_client.wait_for_server(timeout_sec=5.0):
            self.get_logger().error("Action server not available")
            return

        # Send goal
        send_goal_future = self.full_client.send_goal_async(goal_msg, feedback_callback=self.feedback_callback)
        # The code freezes here. It effectively says to ROS: "Process all incoming messages, but don't let my code proceed past this line until the Goal Service replies."
        rclpy.spin_until_future_complete(self, send_goal_future)

        # Get Handle needed to get results from Result Service
        ## the result sof the goal is the goal handle
        goal_handle = send_goal_future.result()

        if not goal_handle.accepted:
            self.get_logger().warn("Goal was rejected by the action server")
            return
        print("goal_handle: ", goal_handle)
        self.get_logger().info("Goal accepted, waiting for result...")

        ## Ask for results of the goal handle 
        result_future = goal_handle.get_result_async()
        # Freeze again. Wait for the robot to drive all the way to the goal.
        rclpy.spin_until_future_complete(self, result_future)

        result = result_future.result()

        # Check goal status symbolically
        if result.status != GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().warn(
                f"Full trajectory action failed (status={result.status})"
            )
            return

        print("result future: ", result_future)
        print("result: ", result)
        # result future:  <rclpy.task.Future object at 0x7352fe9260b0>
        # result:  unh_bc_interfaces.action.FollowFullTrajectory_GetResult_Response(status=4, result=unh_bc_interfaces.action.FollowFullTrajectory_Result(success=True))


        # Access the action result message
        self.get_logger().info(
            f"Full trajectory done: {result.result.success}"
        )


# The Structure (The "Russian Doll")
# When you run result_wrapper = result_future.result(), you get a specialized ROS 2 object (specifically a wrapped_result).

# It contains three specific attributes:

# result_wrapper.status:

# What it is: The Server's final status code.

# Data Type: int (Enum).

# Values: 4 (SUCCEEDED), 5 (CANCELED), 6 (ABORTED).

# Use it to: Check if the action crashed or finished cleanly.

# result_wrapper.goal_id:

# What it is: The unique receipt number for this specific job.

# Data Type: unique_identifier_msgs/UUID.

# Use it to: Log exactly which specific job failed if you sent 50 of them.

# result_wrapper.result (The confusing one!):

# What it is: The User's actual data payload.

# Data Type: The specific Result message defined in your .action file.

# Use it to: Get the actual answer (e.g., "mission_success", "final_pose", "error_message").

def main():
    rclpy.init()
    client = TrajectoryClient()
    # traj1 = [(2,2), (8,2), (8,8)]
    traj2 = [(4,4)]
    
    ## no spinning
    client.send_full_trajectory(traj2)
    # client.send_single_waypoints(traj2)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
