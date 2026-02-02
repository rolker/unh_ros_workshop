#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
import cv2
from rclpy.qos import QoSProfile, QoSReliabilityPolicy, QoSDurabilityPolicy, QoSHistoryPolicy

class CameraViewer(Node):
    def __init__(self):
        super().__init__('stretch_camera_viewer')
        self.bridge = CvBridge()

        qos_profile = QoSProfile(
            reliability=QoSReliabilityPolicy.RELIABLE,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
        )

        # Subscribe to the camera topic
        self.sub = self.create_subscription(
            Image,
            '/camera/camera/color/image_raw',
            self.image_callback,
            qos_profile
        )

        self.get_logger().info("Camera viewer initialized. Press 'q' to quit.")

    def image_callback(self, msg):
        try:
            # Convert ROS Image message to OpenCV image
            cv_image = self.bridge.imgmsg_to_cv2(msg, 'bgr8')
            
        except CvBridgeError as e:
            self.get_logger().warn(f"CV Bridge error: {e}")
            return

        # Display the image in a window
        cv2.imshow('Camera Feed', cv_image)
        key = cv2.waitKey(1) & 0xFF  # Process GUI events

        # If 'q' is pressed, shut down cleanly
        if key == ord('q'):
            self.get_logger().info("Shutting down camera viewer...")
            self.destroy_node()  # Close ROS node
            cv2.destroyAllWindows()  # Close OpenCV window

def main(args=None):
    rclpy.init(args=args)
    node = CameraViewer()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if rclpy.ok():
            node.destroy_node()
        cv2.destroyAllWindows()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
