import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from turtlesim.msg import Pose
from unh_bc_interfaces.action import FollowFullTrajectory
from rclpy.action import ActionServer, CancelResponse, GoalResponse
import math

## for visualization only
from turtlesim.srv import TeleportAbsolute, SetPen

class FullTrajectoryServer(Node):
    ## Its a subclass of Node
    def __init__(self):
        super().__init__('full_trajectory_server')
        
        ## Instantiate a new action server
        self._action_server = ActionServer(
            self,
            FollowFullTrajectory, ## type of action
            'follow_full_trajectory', ## name of action. client will call the action by this name
            self.execute_callback, ## the function that will be called when client cals the server
            goal_callback=self.goal_callback,
            cancel_callback=self.cancel_callback
        )
        
   
        
        ## for our specifc use case we need to publish to cmd for the turtle to move and subscribe to pose to calculate the direction and how much is left to reach destination
        self.pose = None
        self.cmd_pub = self.create_publisher(Twist, '/turtle1/cmd_vel', 10)
        self.pose_sub = self.create_subscription(Pose, '/turtle1/pose', self.pose_callback, 10)
        
    def pose_callback(self, msg):
        self.pose = msg

    def goal_callback(self, goal_request):
        """
        Called when a goal is received.
        """
        
        print(f"Recieved goal {goal_request}")
        self.get_logger().info(f"[FullTrajectoryServer] Recieved goal: {goal_request}")
        # By default, accept all goals
        return GoalResponse.ACCEPT

    def cancel_callback(self, goal_handle):
        """
        Called when a cancel request is received.
        """
        print(f"Recieved goal {goal_handle}")
        self.get_logger().info(f"[FullTrajectoryServer] Cancel requested: {goal_handle}")
        # By default, allow cancel
        return CancelResponse.ACCEPT
    
    def normalize_angle(self, angle):
        while angle > math.pi:
            angle -= 2.0 * math.pi
        while angle < -math.pi:
            angle += 2.0 * math.pi
        return angle

    def execute_callback(self, goal_handle):
        ## get the waypoints to follow
        trajectory = goal_handle.request.waypoints
        
        ## instantiate the feedback message
        feedback_msg = FollowFullTrajectory.Feedback()
        for p in trajectory:
            x = p.x
            y = p.y
            while self.pose is None:
                pass
                
            ## while not close enough, keep moving towards the 
            while math.hypot(x - self.pose.x, y - self.pose.y) > 0.1:
                twist = Twist()
                dx = x - self.pose.x
                dy = y - self.pose.y
                angle_to_goal = math.atan2(dy, dx)
                
                distance = math.hypot(dx, dy)
                twist.linear.x = min(1.5, distance)
                
                angle_error = self.normalize_angle(angle_to_goal - self.pose.theta)
                twist.angular.z = 4.0 * angle_error
                self.cmd_pub.publish(twist)
                
                feedback_msg.distance_remaining = math.hypot(x - self.pose.x, y - self.pose.y)
                goal_handle.publish_feedback(feedback_msg)
        
        # ## stop the turtle
        twist.linear.x = 0.0
        twist.angular.z = 0.0
        self.cmd_pub.publish(twist)
                   
        goal_handle.succeed()
        result = FollowFullTrajectory.Result()
        result.success = True
        return result

def main():
    rclpy.init()
    server = FullTrajectoryServer()
    
    executor = rclpy.executors.MultiThreadedExecutor()
    executor.add_node(server)
    executor.spin()

    rclpy.shutdown()

if __name__ == '__main__':
    main()