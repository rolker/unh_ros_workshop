# ROS Humble Docker Setup with GPU Support

This directory contains Docker configuration for running ROS Humble in a container with NVIDIA GPU support, while keeping ROS Jazzy on the host system.

## Prerequisites

- Docker and Docker Compose installed
- NVIDIA GPU and drivers (tested with RTX 2080 Super, Driver 555.42.06)
- NVIDIA Container Toolkit installed

## Quick Start

1. **Build and start the container:**
   ```bash
   cd docker
   ./start.sh
   ```

2. **Access the container shell:**
   ```bash
   ./shell.sh
   ```

3. **Build your ROS workspace:**
   ```bash
   ./build.sh
   ```
   Or inside the container:
   ```bash
   cd /workspace/ros_bootcamp_ws
   colcon build --symlink-install
   ```

4. **Stop the container:**
   ```bash
   ./stop.sh
   ```

## Container Features

- **ROS Humble Desktop Full** - Complete ROS 2 Humble installation
- **GPU Support** - NVIDIA CUDA 12.1 with PyTorch for deep learning
- **Shared Workspace** - Source code mounted from `../ros_bootcamp_ws/src`
- **Isolated Builds** - Build artifacts kept separate from host Jazzy installation
- **X11 Forwarding** - GUI tools like RViz work inside container
- **Host Networking** - Easy ROS communication between container and host

## Directory Structure

```
docker/
├── Dockerfile           # Container image definition
├── docker-compose.yml   # Container orchestration config
├── entrypoint.sh        # Container startup script
├── bashrc_ros           # Shell customizations
├── .dockerignore        # Files to exclude from build context
├── start.sh            # Start container
├── shell.sh            # Open shell in container
├── build.sh            # Build ROS workspace
├── stop.sh             # Stop container
└── README.md           # This file
```

## Usage Examples

### Run a ROS node in the container
```bash
./shell.sh
# Inside container:
ros2 run your_package your_node
```

### Test GPU access
```bash
./shell.sh
# Inside container:
nvidia-smi
python3 -c "import torch; print(torch.cuda.is_available())"
```

### Run on specific ROS domain
```bash
ROS_DOMAIN_ID=42 ./start.sh
```

### Communication between Host (Jazzy) and Container (Humble)

Since both are on the same network (host mode), they can communicate:

**Host (Jazzy):**
```bash
ros2 topic pub /test std_msgs/msg/String "data: 'Hello from Jazzy'"
```

**Container (Humble):**
```bash
ros2 topic echo /test
```

**Note:** ROS 2 Jazzy and Humble use compatible DDS middleware, but some message types may differ. Test thoroughly.

## Customization

### Change User UID/GID
By default, the container user matches your host UID/GID (1000:1000). To change:
```bash
USER_UID=1001 USER_GID=1001 ./start.sh
```

### Add More Dependencies
Edit `Dockerfile` and rebuild:
```bash
docker compose build --no-cache
```

### Persistent Data
Build artifacts are stored in Docker volumes:
- `humble_build` - Build files
- `humble_install` - Installed packages
- `humble_log` - Log files

To remove them:
```bash
docker compose down -v
```

## Troubleshooting

### GPU not accessible
Check NVIDIA Container Toolkit:
```bash
docker run --rm --gpus all nvidia/cuda:12.1.0-base-ubuntu22.04 nvidia-smi
```

### GUI applications don't work
Allow X11 connections:
```bash
xhost +local:docker
```

### Permission errors on shared files
Ensure UID/GID match between host and container:
```bash
echo "Host UID: $(id -u), GID: $(id -g)"
# Inside container:
id
```

### ROS nodes can't communicate
- Check `ROS_DOMAIN_ID` is the same on host and container
- Verify network mode is `host` in docker-compose.yml
- Check firewall settings

### Build fails with "no space left"
Clean up Docker:
```bash
docker system prune -a
docker volume prune
```

## Technical Details

### Workspace Isolation
- **Source code** (`src/`) is shared between host and container
- **Build artifacts** (`build/`, `install/`, `log/`) are container-specific
- This prevents library conflicts between Jazzy (host) and Humble (container)

### Network Configuration
- Uses `network_mode: host` for simplicity
- Container shares host's network stack
- ROS discovery works automatically

### GPU Configuration
- Uses NVIDIA Container Toolkit
- Full GPU capabilities exposed (compute, utility)
- Compatible with CUDA 12.x applications

## References

- [ROS 2 Humble Documentation](https://docs.ros.org/en/humble/)
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-container-toolkit)
- [Docker Compose Documentation](https://docs.docker.com/compose/)
