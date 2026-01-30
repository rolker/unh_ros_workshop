import cv2
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

class CameraPublisher(Node):
    def __init__(self):
        super().__init__('camera_publisher')
        self.declare_parameter('cam_id', 0)


        cam_id = int(self.get_parameter('cam_id').value)
        self.cap = cv2.VideoCapture(cam_id)
        if not self.cap.isOpened():
            raise RuntimeError(f"Could not open camera id {cam_id}. Try --ros-args -p cam_id:=1")
        self.cap.setExceptionMode(True)

        w = self.cap.get(cv2.CAP_PROP_FRAME_WIDTH)
        h = self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
        fps = self.cap.get(cv2.CAP_PROP_FPS)
        self.get_logger().info(f"Camera default settings: {w}x{h} @ {fps}fps")

        self.declare_parameter('width', w)
        self.declare_parameter('height', h)
        self.declare_parameter('fps', fps)

        w = int(self.get_parameter('width').value)
        h = int(self.get_parameter('height').value)
        fps = float(self.get_parameter('fps').value)


        # self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, w)
        # self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, h)
        # self.cap.set(cv2.CAP_PROP_FPS, fps)

        self.pub = self.create_publisher(Image, '/camera/image_raw', 10)
        self.bridge = CvBridge()
        period = 1.0 / max(fps, 1.0)
        self.timer = self.create_timer(period, self.on_timer)

        self.get_logger().info(f"Publishing /camera/image_raw from cam_id={cam_id} ({w}x{h} @ ~{fps}fps)")

    def on_timer(self):
        ok, frame = self.cap.read()
        if not ok:
            self.get_logger().warn("Frame read failed.")
            is_opened = self.cap.isOpened()
            pos_frames = self.cap.get(cv2.CAP_PROP_POS_FRAMES)
            frame_count = self.cap.get(cv2.CAP_PROP_FRAME_COUNT)
            
            self.get_logger().warn(
                f"Camera opened: {is_opened}, "
                f"Pos: {pos_frames}, "
                f"Total frames: {frame_count}"
            )
            exit
        msg = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
        msg.header.stamp = self.get_clock().now().to_msg()
        self.pub.publish(msg)

def main():
    rclpy.init()
    node = CameraPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.cap.release()
    node.destroy_node()
    rclpy.shutdown()
