# CMake Build Options

The bootcamp_vision_cpp package supports several CMake options to customize the build.

## Available Options

### USE_ONNXRUNTIME (Default: ON)

Controls whether to use ONNXRuntime or OpenCV DNN for inference.

```bash
# Use ONNXRuntime (default)
colcon build --packages-select bootcamp_vision_cpp

# Use OpenCV DNN instead
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DUSE_ONNXRUNTIME=OFF
```

**Note**: OpenCV DNN has known compatibility issues with YOLOv8 ONNX models in version 4.6.0.

### BUILD_ONNXRUNTIME_CUDA (Default: OFF)

Automatically downloads and builds ONNXRuntime from source with CUDA support.

```bash
# Build with CUDA support (30-60 minutes, only needed once)
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON
```

**What it does:**
- Clones ONNXRuntime v1.23.2 from GitHub
- Configures with CUDA support (`--use_cuda`)
- Builds in parallel
- Installs to `thirdparty/onnxruntime/`
- Future builds will detect and use the CUDA-enabled library

**Requirements:**
- CUDA Toolkit (tested with CUDA 12.5)
- cuDNN
- ~10GB free disk space
- 30-60 minutes build time
- GCC/G++ compiler

**Performance Impact:**
- YOLO: ~30 FPS (CPU) → ~30 FPS (GPU, no change needed)
- Face detection: ~0.5 FPS (CPU) → ~30 FPS (GPU, **60x faster!**)

### DOWNLOAD_YOLO_MODELS (Default: ON)

Controls whether models are automatically downloaded during build.

```bash
# Disable auto-download
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DDOWNLOAD_YOLO_MODELS=OFF
```

If disabled, you must manually download models:
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws/src/bootcamp_vision_cpp
./scripts/setup_models.sh
```

## Common Build Scenarios

### First Time Build (CPU only)
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp
```

### Build with GPU Support
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
source /home/roland/unh_ros_workshop/underlay_ws/install/setup.bash
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON
```

**Important**: This only needs to be done once. Subsequent builds will automatically detect the CUDA-enabled library:
```bash
# Future builds after CUDA is set up
colcon build --packages-select bootcamp_vision_cpp
```

### Clean Rebuild
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws
rm -rf build/bootcamp_vision_cpp install/bootcamp_vision_cpp
colcon build --packages-select bootcamp_vision_cpp
```

### Rebuild ONNXRuntime from Scratch
```bash
cd ~/unh_ros_workshop/ros_bootcamp_ws/src/bootcamp_vision_cpp
rm -rf thirdparty/onnxruntime*
cd ~/unh_ros_workshop/ros_bootcamp_ws
colcon build --packages-select bootcamp_vision_cpp \
  --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON
```

## Checking Build Configuration

After building, CMake will display the configuration:

```
-- Using ONNXRuntime from .../thirdparty/onnxruntime
--   ONNXRuntime CUDA provider: FOUND
```

Or for CPU-only:
```
-- Using ONNXRuntime from .../thirdparty/onnxruntime
--   ONNXRuntime CUDA provider: NOT FOUND (CPU-only)
--   To enable GPU: colcon build --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON
```

## Troubleshooting

### Build takes too long
The ONNXRuntime CUDA build takes 30-60 minutes. This is normal and only happens once.

### Out of disk space
ONNXRuntime build requires ~10GB. Check available space:
```bash
df -h
```

### CUDA not found
Ensure CUDA toolkit is installed:
```bash
ls -l /usr/local/cuda
nvcc --version
```

### Build fails with CUDA errors
Check that cuDNN is installed:
```bash
ls -l /usr/lib/x86_64-linux-gnu/libcudnn*
```

### Want to switch between CPU and GPU
Just rebuild ONNXRuntime:
```bash
# Switch to GPU
colcon build --cmake-args -DBUILD_ONNXRUNTIME_CUDA=ON

# Switch to CPU
cd src/bootcamp_vision_cpp
rm -rf thirdparty/onnxruntime
./scripts/setup_onnxruntime.sh  # Downloads CPU version
colcon build --packages-select bootcamp_vision_cpp
```
