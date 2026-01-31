# GPU Acceleration for Face Detection

## Problem

Face detection with InsightFace models (SCRFD + ArcFace) is **extremely slow on CPU**:
- CPU performance: ~0.5 FPS (2 seconds per frame)
- Required for real-time: 30 FPS

## Solution

Build ONNXRuntime from source with CUDA support.

## Steps

### 1. Build ONNXRuntime with CUDA

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws/src/bootcamp_vision_cpp
./scripts/build_onnxruntime_cuda.sh
```

This will:
- Clone ONNXRuntime v1.23.2
- Build with CUDA 12.5 support
- Install to `thirdparty/onnxruntime/`
- Takes 30-60 minutes

### 2. Rebuild Package

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp
```

### 3. Enable Face Detection

Edit `launch/vision.launch.py`:
```python
'enable_face_detection': True,  # Change from False
```

### 4. Test

```bash
source install/setup.bash
ros2 launch bootcamp_vision_cpp vision.launch.py
```

Check GPU usage:
```bash
nvidia-smi  # Should show vision_node using GPU
```

## Expected Performance

| Backend | YOLO FPS | Face Detection FPS |
|---------|----------|-------------------|
| CPU     | ~30      | ~0.5 (unusable)   |
| GPU     | ~60      | ~30 (real-time)   |

## Requirements

- NVIDIA GPU (you have RTX 2080)
- CUDA 12.x (installed: 12.5)
- cuDNN (check with `dpkg -l | grep cudnn`)
- ~10GB disk space for build
- 30-60 minutes build time

## Why Pre-built Binaries Don't Work

Microsoft's pre-built ONNXRuntime GPU binaries don't include the CUDA provider library (`libonnxruntime_providers_cuda.so`). You must build from source to get full GPU support.
