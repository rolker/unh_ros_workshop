import pickle
import json
import numpy as np
import cv2
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String
from cv_bridge import CvBridge

from ultralytics import YOLO
from insightface.app import FaceAnalysis

def cosine_sim(a: np.ndarray, b: np.ndarray, eps: float = 1e-8) -> float:
    a = a.astype(np.float32)
    b = b.astype(np.float32)
    return float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b) + eps))

class VisionNode(Node):
    def __init__(self):
        super().__init__('vision_node')
        self.declare_parameter('yolo_model', 'yolov8n.pt')
        self.declare_parameter('yolo_conf', 0.35)
        self.declare_parameter('face_thresh', 0.45)

        self.bridge = CvBridge()

        # YOLO
        model_name = self.get_parameter('yolo_model').value
        self.yolo_conf = float(self.get_parameter('yolo_conf').value)
        self.yolo = YOLO(model_name)
        self.get_logger().info(f"YOLO loaded: {model_name}")

        # InsightFace (CPU default)
        self.face_thresh = float(self.get_parameter('face_thresh').value)
        self.face_app = FaceAnalysis(name="buffalo_l", providers=["CPUExecutionProvider"])
        self.face_app.prepare(ctx_id=-1, det_size=(640, 640))
        self.get_logger().info("InsightFace loaded (buffalo_l).")

        # Face DB (optional): fill with your enrolled embeddings later
        # For bootcamp simplicity, we keep it empty by default.
        with open("face_db.pkl", "rb") as f:
            self.face_db = pickle.load(f)
        
        self.sub = self.create_subscription(Image, '/camera/image_raw', self.on_image, 10)
        self.pub_img = self.create_publisher(Image, '/vision/annotated', 10)
        self.pub_det = self.create_publisher(String, '/vision/detections', 10)

        self.get_logger().info("Subscribed to /camera/image_raw")
        self.get_logger().info("Publishing /vision/annotated (Image) and /vision/detections (String JSON)")

    def recognize(self, emb: np.ndarray):
        if not self.face_db:
            return "unknown", -1.0
        best_name, best_score = "unknown", -1.0
        for name, ref in self.face_db.items():
            s = cosine_sim(emb, ref)
            if s > best_score:
                best_score, best_name = s, name
        if best_score < self.face_thresh:
            return "unknown", best_score
        return best_name, best_score

    def on_image(self, msg: Image):
        frame = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')

        # --- Face ---
        faces = self.face_app.get(frame)
        face_out = []
        for f in faces:
            x1, y1, x2, y2 = f.bbox.astype(int).tolist()
            name, score = self.recognize(f.embedding)
            face_out.append({"bbox":[x1,y1,x2,y2], "name":name, "score":float(score)})
            color = (0,255,0) if name != "unknown" else (0,0,255)
            cv2.rectangle(frame, (x1,y1), (x2,y2), color, 2)
            cv2.putText(frame, f"{name} {score:.2f}", (x1, max(20, y1-10)),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2, cv2.LINE_AA)

        # --- YOLO ---
        results = self.yolo.predict(frame, conf=self.yolo_conf, verbose=False)[0]
        yolo_out = []
        boxes = results.boxes
        names = results.names
        if boxes is not None:
            for b in boxes:
                x1, y1, x2, y2 = b.xyxy[0].tolist()
                cls = int(b.cls[0].item())
                conf = float(b.conf[0].item())
                label = names.get(cls, str(cls))
                yolo_out.append({"bbox":[x1,y1,x2,y2], "label":label, "conf":conf})
        frame = results.plot(img=frame)

        # Publish detections as JSON string
        det_msg = String()
        det_msg.data = json.dumps({
            "stamp": {"sec": msg.header.stamp.sec, "nanosec": msg.header.stamp.nanosec},
            "faces": face_out,
            "objects": yolo_out
        })
        self.pub_det.publish(det_msg)

        # Publish annotated image
        out = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
        out.header = msg.header
        self.pub_img.publish(out)

def main():
    rclpy.init()
    node = VisionNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()
