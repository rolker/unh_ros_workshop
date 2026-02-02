#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray
from sensor_msgs.msg import JointState
from hello_helpers.joint_qpos_conversion import get_Idx, JointStateMapping
import numpy as np
import math
import time


class HeadPosePublisher(Node):
    def __init__(self):
        super().__init__('head_pose_publisher')

        # Publisher for joint commands
        self.pub = self.create_publisher(
            Float64MultiArray,
            'joint_pose_cmd',
            10
        )

        # Subscriber for joint states
        self.joint_state = JointState()
        self.create_subscription(
            JointState,
            '/stretch/joint_states',
            self.joint_state_cb,
            10
        )

        # Joint index helper
        self.Idx = get_Idx('eoa_wrist_dw3_tool_sg3')

    def joint_state_cb(self, msg):
        self.joint_state = msg

    def get_current_qpos(self):
        """Return current joint vector"""
        qpos = np.zeros(self.Idx.num_joints)

        name_to_pos = dict(zip(self.joint_state.name, self.joint_state.position))

        qpos[self.Idx.HEAD_PAN] = name_to_pos.get(
            JointStateMapping.ROS_HEAD_PAN, 0.0
        )
        qpos[self.Idx.HEAD_TILT] = name_to_pos.get(
            JointStateMapping.ROS_HEAD_TILT, 0.0
        )

        return qpos

    def publish_qpos(self, qpos):
        msg = Float64MultiArray()
        msg.data = list(qpos)
        self.pub.publish(msg)


def main():
    rclpy.init()
    node = HeadPosePublisher()

    # Wait for first joint_state
    while rclpy.ok() and not node.joint_state.name:
        rclpy.spin_once(node, timeout_sec=0.1)

    qpos = node.get_current_qpos()

    # ---- Desired head pose ----
    qpos[node.Idx.HEAD_PAN] = 0.0                 # rotate head (radians)
    qpos[node.Idx.HEAD_TILT] = math.pi / 4        # 45 degrees

    node.publish_qpos(qpos)

    # Give time for command to be sent
    time.sleep(0.5)

    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
