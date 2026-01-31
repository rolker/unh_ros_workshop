#!/bin/bash
# Post-build script to download and setup YOLO models for bootcamp_vision_cpp
# Automatically creates a temporary venv if ultralytics is not available

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PKG_DIR="$(dirname "$SCRIPT_DIR")"

# Determine model directory
if [ -n "$ROS_PACKAGE_PATH" ]; then
    # If in ROS environment, use install space
    MODEL_DIR="$PKG_DIR/../install/bootcamp_vision_cpp/share/bootcamp_vision_cpp/models"
else
    # Otherwise use source directory
    MODEL_DIR="$PKG_DIR/models"
fi

echo "=========================================="
echo "YOLO Model Setup for bootcamp_vision_cpp"
echo "=========================================="
echo "Model directory: $MODEL_DIR"
echo ""

# Create models directory
mkdir -p "$MODEL_DIR"

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found!"
    echo "Please install Python 3"
    exit 1
fi

# Check if model already exists
if [ -f "$MODEL_DIR/yolov8n.onnx" ]; then
    echo "✓ Model already exists: $MODEL_DIR/yolov8n.onnx"
    echo "Skipping download. Delete the file to re-download."
    exit 0
fi

# Function to run the download script
run_download() {
    local python_cmd=$1
    echo "Downloading YOLOv8n model..."
    $python_cmd "$SCRIPT_DIR/download_yolo_models.py" --model yolov8n --output-dir "$MODEL_DIR"
}

# Check for ultralytics in system Python
if python3 -c "import ultralytics" 2>/dev/null; then
    echo "✓ ultralytics found in system Python"
    run_download python3
else
    echo "ultralytics not found in system Python"
    echo "Creating temporary virtual environment..."
    
    # Create temporary venv
    TEMP_VENV=$(mktemp -d -t yolo-venv-XXXXXX)
    trap "rm -rf $TEMP_VENV" EXIT
    
    echo "  Location: $TEMP_VENV"
    python3 -m venv "$TEMP_VENV"
    
    # Activate and install ultralytics
    source "$TEMP_VENV/bin/activate"
    
    echo "Installing ultralytics in temporary venv..."
    pip install --quiet --upgrade pip
    pip install --quiet ultralytics
    
    if [ $? -eq 0 ]; then
        echo "✓ ultralytics installed successfully"
        run_download python3
        deactivate
    else
        deactivate
        echo "ERROR: Failed to install ultralytics"
        exit 1
    fi
    
    # Cleanup happens automatically via trap
    echo "Cleaning up temporary venv..."
fi

if [ -f "$MODEL_DIR/yolov8n.onnx" ]; then
    MODEL_SIZE=$(du -h "$MODEL_DIR/yolov8n.onnx" | cut -f1)
    echo ""
    echo "✓ Setup complete!"
    echo ""
    echo "Model: $MODEL_DIR/yolov8n.onnx ($MODEL_SIZE)"
    echo ""
    echo "To use with vision_node:"
    echo "  ros2 run bootcamp_vision_cpp vision_node --ros-args \\"
    echo "    -p yolo_model:=$MODEL_DIR/yolov8n.onnx"
    echo ""
    echo "Or use the launch file:"
    echo "  ros2 launch bootcamp_vision_cpp vision.launch.py"
else
    echo ""
    echo "✗ Setup failed! Model file not created."
    exit 1
fi

