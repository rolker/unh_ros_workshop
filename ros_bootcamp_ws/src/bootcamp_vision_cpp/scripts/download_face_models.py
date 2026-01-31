#!/usr/bin/env python3
"""
Script to download InsightFace models for face detection and recognition.
Downloads the buffalo_l model pack and extracts the ONNX files we need.
Automatically creates a temporary venv if insightface is not available.
"""

import os
import sys
import argparse
import subprocess
import tempfile
import shutil
from pathlib import Path


def setup_insightface_venv():
    """Create a temporary venv with insightface installed."""
    print("insightface not found in system Python", file=sys.stderr)
    print("Creating temporary virtual environment...", file=sys.stderr)
    
    temp_dir = tempfile.mkdtemp(prefix='insightface-venv-')
    venv_python = os.path.join(temp_dir, 'bin', 'python')
    
    try:
        # Create venv
        print(f"  Location: {temp_dir}", file=sys.stderr)
        subprocess.run([sys.executable, '-m', 'venv', temp_dir], 
                      check=True, capture_output=True)
        
        # Upgrade pip quietly
        print("Installing insightface in temporary venv...", file=sys.stderr)
        subprocess.run([venv_python, '-m', 'pip', 'install', 
                       '--quiet', '--upgrade', 'pip'],
                      check=True, capture_output=True)
        
        # Install insightface and onnxruntime
        subprocess.run([venv_python, '-m', 'pip', 'install', 
                       '--quiet', 'insightface', 'onnxruntime'],
                      check=True, capture_output=True)
        
        print("✓ insightface installed successfully", file=sys.stderr)
        return temp_dir, venv_python
        
    except subprocess.CalledProcessError as e:
        shutil.rmtree(temp_dir, ignore_errors=True)
        raise RuntimeError(f"Failed to setup venv: {e}")


def download_insightface_models(output_dir=None, use_venv=True):
    """
    Download InsightFace buffalo_l model pack.
    
    Args:
        output_dir: Directory to save ONNX models (default: ./models)
        use_venv: If True, create temp venv if insightface not available
    """
    # Check if insightface is available
    try:
        import insightface
        from insightface.app import FaceAnalysis
        venv_dir = None
        print("Using system insightface", file=sys.stderr)
    except ImportError:
        if not use_venv:
            raise RuntimeError("insightface not installed and use_venv=False")
        
        venv_dir, venv_python = setup_insightface_venv()
        
        # Re-run this script with venv Python
        print("Re-running with venv Python...", file=sys.stderr)
        args = [venv_python, __file__]
        if output_dir:
            args.extend(['--output-dir', output_dir])
        args.append('--no-venv')
        
        result = subprocess.run(args, check=True)
        
        # Cleanup venv
        print("Cleaning up temporary venv...", file=sys.stderr)
        shutil.rmtree(venv_dir, ignore_errors=True)
        return
    
    # Now we're running with insightface available
    output_dir = Path(output_dir) if output_dir else Path('./models')
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Downloading InsightFace buffalo_l models...", file=sys.stderr)
    
    # Initialize FaceAnalysis to trigger model download
    app = FaceAnalysis(name='buffalo_l', providers=['CPUExecutionProvider'])
    app.prepare(ctx_id=0, det_size=(640, 640))
    
    # Copy the models we need from ~/.insightface/models/buffalo_l
    source_dir = Path.home() / '.insightface' / 'models' / 'buffalo_l'
    
    models_to_copy = {
        'det_10g.onnx': 'Face detection (SCRFD)',
        'w600k_r50.onnx': 'Face recognition (ArcFace)'
    }
    
    print(f"\nCopying models to {output_dir}:", file=sys.stderr)
    for model_file, description in models_to_copy.items():
        src = source_dir / model_file
        dst = output_dir / model_file
        
        if not src.exists():
            raise FileNotFoundError(f"Model {model_file} not found at {src}")
        
        # Copy model
        shutil.copy2(src, dst)
        size_mb = dst.stat().st_size / (1024 * 1024)
        print(f"  ✓ {model_file}: {size_mb:.1f} MB ({description})", file=sys.stderr)
    
    print(f"\n✓ InsightFace models downloaded successfully", file=sys.stderr)
    print(f"  Output directory: {output_dir.absolute()}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description='Download InsightFace models for face detection and recognition'
    )
    parser.add_argument(
        '--output-dir', 
        type=str, 
        default='./models',
        help='Output directory for ONNX models (default: ./models)'
    )
    parser.add_argument(
        '--no-venv',
        action='store_true',
        help='Do not create temporary venv (fail if insightface not installed)'
    )
    
    args = parser.parse_args()
    
    try:
        download_insightface_models(
            output_dir=args.output_dir,
            use_venv=not args.no_venv
        )
        sys.exit(0)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
