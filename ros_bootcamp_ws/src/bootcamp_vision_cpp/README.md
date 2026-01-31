# bootcamp_vision_cpp

ROS2 vision package in C++ featuring camera image publishing and computer vision processing with **runtime-selectable inference backends**.

## 📋 Current Status

✅ **Complete**: Camera publisher, YOLO detection, face detection/recognition  
✅ **Backend Architecture**: Runtime-selectable backends (OpenCV DNN, ONNXRuntime)  
✅ **Default**: OpenCV DNN with YOLOv5 (no external dependencies)  
✅ **YOLO**: YOLOv5 works with OpenCV DNN (~30 FPS on CPU)  
⚠️ **YOLOv8**: Requires ONNXRuntime backend (optional)  
⚠️ **Face Detection**: Requires ONNXRuntime + CUDA (~30 FPS with GPU)  
💡 **GPU Acceleration**: Optional ONNXRuntime build with CUDA support  

### Inference Backends

The package supports **runtime backend selection** without recompiling:

| Backend | YOLO | Faces | CUDA | Status |
|---------|------|-------|------|--------|
| **OpenCV DNN** | ✅ YOLOv5 | ❌ | ❌ | **Default** (CPU, no deps) |
| **ONNXRuntime + CUDA** | ✅ YOLOv8 | ✅ 30 FPS | ✅ | Optional (requires build) |
| **ONNXRuntime CPU** | ✅ YOLOv8 | ⚠️ 0.5 FPS | ❌ | Optional |

**Default behavior**: Uses OpenCV DNN with YOLOv5 models (best compatibility with OpenCV 4.6.0).

**Usage**:
```bash
# Auto-select best backend (default: OpenCV DNN)
ros2 launch bootcamp_vision_cpp vision.launch.py

# Force specific backend
ros2 launch bootcamp_vision_cpp vision.launch.py backend:=opencv
ros2 launch bootcamp_vision_cpp vision.launch.py backend:=onnxruntime
```

### YOLO Model Support

- **YOLOv5**: Recommended for OpenCV DNN backend (default)
- **YOLOv8**: Requires ONNXRuntime backend for best compatibility

### Enable ONNXRuntime (Optional)

ONNXRuntime is **disabled by default**. To enable it:

**Build with ONNXRuntime + CUDA** (for GPU-accelerated face detection):
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DUSE_ONNXRUNTIME=ON -DBUILD_ONNXRUNTIME_CUDA=ON
# First build takes 30-60 min for CUDA support
# Subsequent builds are fast (~10s)
```

**Build with ONNXRuntime CPU only**:
```bash
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DUSE_ONNXRUNTIME=ON -DBUILD_ONNXRUNTIME_CUDA=OFF
```

See [docs/BUILD_ONNXRUNTIME_CUDA.md](docs/BUILD_ONNXRUNTIME_CUDA.md) and [docs/CMAKE_OPTIONS.md](docs/CMAKE_OPTIONS.md) for details.

---

## Overview

This package provides two main nodes:
- **camera_pub_node**: Publishes camera frames to ROS2 topics
- **vision_node**: Performs YOLO object detection and (optionally) face detection/recognition

This is a C++ implementation parallel to the Python `bootcamp_vision` package, using a **plugin-style backend architecture** for flexible inference engine selection.

## Features

- **Runtime Backend Selection**: Choose inference engine at runtime (no recompilation)
- **Auto Backend Discovery**: Automatically selects best available backend
- **Camera capture and publishing** (`/camera/image_raw`)
- **YOLOv8 object detection** using ONNX models with per-class color coding
- **Face detection and recognition** (InsightFace SCRFD + ArcFace)
- **Separate image topics**: `/vision/annotated` (combined), `/vision/faces` (faces only)
- **JSON detection results** (`/vision/detections`)
- **Configurable parameters** via ROS2 parameter system
- **Automatic model download** during build
- **GPU acceleration** with automatic CUDA build

## Dependencies

- ROS2 Humble (or compatible)
- OpenCV 4.x
- cv_bridge
- sensor_msgs
- std_msgs
- rclcpp
- ONNXRuntime (auto-downloaded and built with CUDA by default)

## Building

**Basic build (OpenCV DNN with YOLOv5 - default):**
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp
source install/setup.bash
```

**Build with ONNXRuntime for YOLOv8 and face detection:**
```bash
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DUSE_ONNXRUNTIME=ON -DBUILD_ONNXRUNTIME_CUDA=ON
```

See [docs/CMAKE_OPTIONS.md](docs/CMAKE_OPTIONS.md) for all build options.

## Usage

### Quick Start

1. Build and setup:
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp
source install/setup.bash
```

2. Run with launch file:
```bash
ros2 launch bootcamp_vision_cpp vision.launch.py
```

### Backend Selection

The package automatically selects the best available backend. You can override this:

**Auto-select (default)**:
```bash
ros2 launch bootcamp_vision_cpp vision.launch.py
# Chooses: OpenCV DNN > ONNXRuntime+CUDA > ONNXRuntime CPU
```

**Force specific backend**:
```bash
ros2 launch bootcamp_vision_cpp vision.launch.py backend:=onnxruntime
ros2 launch bootcamp_vision_cpp vision.launch.py backend:=opencv
```

**Check available backends**:
```bash
ros2 run bootcamp_vision_cpp vision_node --ros-args -p backend:=invalid
# Lists available backends in error message
```

### Running Individual Nodes

**Camera Publisher:**
```bash
ros2 run bootcamp_vision_cpp camera_pub_node --ros-args \
  -p cam_id:=0 \
  -p fps:=30.0
```

**Vision Node with backend selection:**
```bash
ros2 run bootcamp_vision_cpp vision_node --ros-args \
  -p backend:=auto \
  -p yolo_model:=/path/to/yolov8n.onnx \
  -p yolo_conf:=0.35
```

### Using Launch File

```bash
ros2 launch bootcamp_vision_cpp vision.launch.py
```

## Model Setup

### YOLO Model (Required)

**YOLOv5 (Recommended for OpenCV DNN - default)**:

The package downloads YOLOv5n automatically during build. To manually download other YOLOv5 variants:

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws/src/bootcamp_vision_cpp
./scripts/download_yolo_models.py --model yolov5s --output-dir models
```

**Available YOLOv5 models:**
- `yolov5n` - Nano (fastest, default)
- `yolov5s` - Small
- `yolov5m` - Medium
- `yolov5l` - Large
- `yolov5x` - Extra Large (slowest, most accurate)

**YOLOv8 (Requires ONNXRuntime backend)**:

To use YOLOv8 models, you need to build with ONNXRuntime:

```bash
# Download YOLOv8 model
./scripts/download_yolo_models.py --model yolov8n --output-dir models

# Build with ONNXRuntime
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DUSE_ONNXRUNTIME=ON
```

**Available YOLOv8 models:**
- `yolov8n` - Nano (fastest, least accurate)
- `yolov8s` - Small
- `yolov8m` - Medium
- `yolov8l` - Large
- `yolov8x` - Extra Large (slowest, most accurate)

### Face Detection Model (Optional)

Face detection can be added by providing a face detection model:

```bash
ros2 run bootcamp_vision_cpp vision_node --ros-args \
  -p face_model:=/path/to/face_detector.onnx
```

Recommended models:
- SCRFD (ONNX format)
- RetinaFace (ONNX format)
- OpenCV's default face detector (Caffe format)

## Parameters

### camera_pub_node

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `cam_id` | int | 0 | Camera device ID |
| `width` | double | 0.0 | Camera width (0 = use default) |
| `height` | double | 0.0 | Camera height (0 = use default) |
| `fps` | double | 30.0 | Target frame rate |

### vision_node

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `backend` | string | "auto" | Inference backend: auto, opencv, onnxruntime |
| `yolo_model` | string | "yolov5n.onnx" | Path to YOLO ONNX model |
| `yolo_conf` | double | 0.35 | YOLO confidence threshold |
| `face_det_model` | string | "det_10g.onnx" | Path to face detection model (SCRFD) |
| `face_rec_model` | string | "w600k_r50.onnx" | Path to face recognition model (ArcFace) |
| `face_thresh` | double | 0.45 | Face recognition threshold |
| `enable_face_detection` | bool | false | Enable face detection (requires ONNXRuntime+CUDA) |
| `face_db_path` | string | "" | Path to face database JSON (optional) |

## Topics

### Published

- `/camera/image_raw` (sensor_msgs/Image) - Raw camera frames from camera_pub_node
- `/vision/annotated` (sensor_msgs/Image) - Combined annotated frames (YOLO + faces)
- `/vision/faces` (sensor_msgs/Image) - Face-only annotations
- `/vision/detections` (std_msgs/String) - JSON string with detection data

### Subscribed

- `/camera/image_raw` (sensor_msgs/Image) - Input for vision processing

## Detection Output Format

The `/vision/detections` topic publishes JSON in the following format:

```json
{
  "stamp": {
    "sec": 123456,
    "nanosec": 789012345
  },
  "faces": [
    {
      "bbox": [x1, y1, x2, y2],
      "name": "unknown",
      "score": 0.95
    }
  ],
  "objects": [
    {
      "bbox": [x1, y1, x2, y2],
      "label": "person",
      "conf": 0.87
    }
  ]
}
```

## Performance Notes

- The C++ implementation provides better performance than the Python equivalent
- CPU inference is default; GPU support requires OpenCV built with CUDA
- Lower confidence thresholds increase detection sensitivity but may produce false positives
- Image resolution affects inference speed (640x640 is a good balance)

## Troubleshooting

**Camera not opening:**
- Try different `cam_id` values (0, 1, 2, etc.)
- Check camera permissions: `ls -l /dev/video*`

**YOLO model not loading:**
- Ensure the ONNX file exists and path is correct
- Verify OpenCV was built with ONNX support: `cv::dnn::getAvailableBackends()`

**Low frame rate:**
- Reduce camera resolution
- Use a smaller YOLO model (e.g., yolov8n instead of yolov8x)
- Lower the confidence threshold to reduce post-processing

## Future Enhancements

- [ ] GPU/CUDA support for faster inference
- [ ] Face recognition with embedding-based matching
- [ ] Multi-camera support
- [ ] Dynamic model switching
- [ ] Performance metrics publishing

## License

Apache-2.0

## Author

ROS2 Bootcamp Workshop
