#!/usr/bin/env python3
"""
Script to download and convert YOLO models (v5 or v8) to ONNX format for use with OpenCV DNN.
YOLOv5 is recommended for OpenCV DNN backend with OpenCV 4.6.0 or lower.
This runs automatically during package build or can be run manually.
Automatically creates a temporary venv if ultralytics is not available.
"""

import os
import sys
import argparse
import subprocess
import tempfile
import shutil
from pathlib import Path


def setup_ultralytics_venv():
    """Create a temporary venv with ultralytics installed."""
    print("ultralytics not found in system Python", file=sys.stderr)
    print("Creating temporary virtual environment...", file=sys.stderr)
    
    temp_dir = tempfile.mkdtemp(prefix='yolo-venv-')
    venv_python = os.path.join(temp_dir, 'bin', 'python')
    
    try:
        # Create venv
        print(f"  Location: {temp_dir}", file=sys.stderr)
        subprocess.run([sys.executable, '-m', 'venv', temp_dir], 
                      check=True, capture_output=True)
        
        # Upgrade pip quietly
        print("Installing ultralytics and dependencies in temporary venv...", file=sys.stderr)
        subprocess.run([venv_python, '-m', 'pip', 'install', 
                       '--quiet', '--upgrade', 'pip'],
                      check=True, capture_output=True)
        
        # Install ultralytics and ONNX dependencies
        subprocess.run([venv_python, '-m', 'pip', 'install', 
                       '--quiet', 'ultralytics', 'onnx', 'onnxslim', 'onnxruntime'],
                      check=True, capture_output=True)
        
        print("✓ ultralytics and ONNX tools installed successfully", file=sys.stderr)
        return temp_dir, venv_python
        
    except subprocess.CalledProcessError as e:
        shutil.rmtree(temp_dir, ignore_errors=True)
        raise RuntimeError(f"Failed to setup venv: {e}")


def download_and_convert_yolo(model_name='yolov8n', output_dir=None, use_venv=True):
    """
    Download a YOLO model and convert it to ONNX format.
    
    Args:
        model_name: Name of the model (e.g., 'yolov8n', 'yolov5n', 'yolov5s')
        output_dir: Directory to save the ONNX model (default: current directory)
        use_venv: Whether to create a temporary venv if ultralytics is not available
    """
    # Check if ultralytics is available
    ultralytics_available = False
    try:
        import ultralytics
        ultralytics_available = True
        print("✓ ultralytics found in system Python", file=sys.stderr)
    except ImportError:
        pass
    
    # Setup venv if needed
    temp_venv = None
    cleanup_venv = False
    
    if not ultralytics_available:
        if not use_venv:
            print("ERROR: ultralytics package not found.", file=sys.stderr)
            print("Install with: pip install ultralytics", file=sys.stderr)
            return False
        
        try:
            temp_venv, venv_python = setup_ultralytics_venv()
            cleanup_venv = True
            
            # Re-run this script with the venv Python
            cmd = [venv_python, __file__, '--model', model_name, '--no-venv']
            if output_dir:
                cmd.extend(['--output-dir', str(output_dir)])
            
            result = subprocess.run(cmd, check=False)
            
            # Cleanup
            print("Cleaning up temporary venv...", file=sys.stderr)
            shutil.rmtree(temp_venv, ignore_errors=True)
            
            return result.returncode == 0
            
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            if temp_venv and cleanup_venv:
                shutil.rmtree(temp_venv, ignore_errors=True)
            return False
    
    # If we're here, ultralytics is available - do the actual work
    try:
        from ultralytics import YOLO
    except ImportError:
        print("ERROR: Failed to import ultralytics", file=sys.stderr)
        return False

    if output_dir is None:
        output_dir = Path.cwd()
    else:
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

    pt_model = f"{model_name}.pt"
    onnx_model = output_dir / f"{model_name}.onnx"

    # Check if ONNX model already exists
    if onnx_model.exists():
        print(f"✓ ONNX model already exists: {onnx_model}", file=sys.stderr)
        return True

    print(f"Downloading and converting {pt_model} to ONNX...", file=sys.stderr)
    print(f"Output directory: {output_dir}", file=sys.stderr)

    try:
        # Load model (will download if not present)
        print(f"Loading YOLO model: {pt_model}", file=sys.stderr)
        model = YOLO(pt_model)

        # Determine if this is YOLOv5 or YOLOv8
        is_yolov5 = model_name.startswith('yolov5')
        
        # Export to ONNX with appropriate settings
        if is_yolov5:
            # YOLOv5 works well with OpenCV DNN using opset 12
            print("Exporting YOLOv5 to ONNX format with opset=12 (OpenCV DNN compatible)...", file=sys.stderr)
            export_path = model.export(
                format='onnx', 
                imgsz=640,  # Single value for square input
                simplify=True, 
                opset=12,
                dynamic=False  # Static shapes for OpenCV DNN
            )
        else:
            # YOLOv8 - use opset 12 for best OpenCV compatibility
            print("Exporting YOLOv8 to ONNX format with opset=12 (OpenCV DNN compatible)...", file=sys.stderr)
            print("Note: YOLOv5 is recommended for OpenCV DNN backend", file=sys.stderr)
            export_path = model.export(
                format='onnx', 
                imgsz=640,
                simplify=True, 
                opset=12,
                dynamic=False
            )
        
        # Move to output directory if needed
        export_path = Path(export_path)
        if export_path.parent != output_dir:
            final_path = output_dir / export_path.name
            export_path.rename(final_path)
            print(f"Moved to: {final_path}", file=sys.stderr)
        else:
            final_path = export_path

        print(f"✓ Successfully created: {final_path}", file=sys.stderr)
        print(f"  Model size: {final_path.stat().st_size / 1024 / 1024:.1f} MB", file=sys.stderr)
        return True

    except Exception as e:
        print(f"ERROR: Failed to download/convert model: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Download and convert YOLO models (v5 or v8) to ONNX format'
    )
    parser.add_argument(
        '--model',
        default='yolov5n',
        choices=['yolov5n', 'yolov5s', 'yolov5m', 'yolov5l', 'yolov5x',
                 'yolov8n', 'yolov8s', 'yolov8m', 'yolov8l', 'yolov8x'],
        help='YOLO model variant (default: yolov5n - recommended for OpenCV DNN)'
    )
    parser.add_argument(
        '--output-dir',
        default=None,
        help='Output directory for ONNX model (default: current directory)'
    )
    parser.add_argument(
        '--all',
        action='store_true',
        help='Download all model variants (n, s, m, l, x) for selected version'
    )
    parser.add_argument(
        '--version',
        default='v5',
        choices=['v5', 'v8'],
        help='YOLO version to download when using --all (default: v5)'
    )
    parser.add_argument(
        '--no-venv',
        action='store_true',
        help='Do not create temporary venv (internal use)'
    )

    args = parser.parse_args()

    if args.all:
        # Determine which version to download
        prefix = 'yolov5' if args.version == 'v5' else 'yolov8'
        models = [f'{prefix}n', f'{prefix}s', f'{prefix}m', f'{prefix}l', f'{prefix}x']
        print(f"Downloading all YOLO{args.version} model variants...", file=sys.stderr)
        success_count = 0
        for model in models:
            print(f"\n{'='*60}", file=sys.stderr)
            print(f"Processing {model}...", file=sys.stderr)
            print('='*60, file=sys.stderr)
            if download_and_convert_yolo(model, args.output_dir, use_venv=not args.no_venv):
                success_count += 1
        
        print(f"\n{'='*60}", file=sys.stderr)
        print(f"Completed: {success_count}/{len(models)} models converted successfully", file=sys.stderr)
        return success_count == len(models)
    else:
        return download_and_convert_yolo(args.model, args.output_dir, use_venv=not args.no_venv)


if __name__ == '__main__':
    success = main()
    sys.exit(0 if success else 1)
