# Building ONNXRuntime with CUDA Support

## Quick Start

### Option 1: Automatic CMake Build (Recommended)

Let CMake handle everything automatically:

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON
```

This downloads, builds, and installs ONNXRuntime with CUDA support (30-60 minutes). Only needs to be done once.

### Option 2: Manual Script

Run the automated build script (takes 30-60 minutes):

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws/src/bootcamp_vision_cpp
./scripts/build_onnxruntime_cuda.sh
```

Then rebuild your package:

```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp
```

Enable face detection in `launch/vision.launch.py` and test!

## Performance

- **CPU**: 0.5 FPS (unusable)
- **GPU**: ~30 FPS (real-time)
