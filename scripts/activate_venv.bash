#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$SCRIPT_DIR/.."

set -ex # exit on error, show commands

if [ ! -d "$BASE_DIR/.venv" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv "$BASE_DIR/.venv"
  source "$BASE_DIR/.venv/bin/activate"
  echo "Upgrading pip..."
  python -m pip install --upgrade pip
  echo "Installing requirements..."
  pip install -r "$BASE_DIR/requirements.txt"
  echo "Virtual environment created and activated!"
else
  source "$BASE_DIR/.venv/bin/activate"
  echo "Virtual environment activated"
fi


