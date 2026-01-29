# Docker Quick Reference

## Start Container
```bash
cd docker
./start.sh
```

## Access Container Shell
```bash
cd docker
./shell.sh
```

## Build ROS Workspace
```bash
cd docker
./build.sh
```

## Stop Container
```bash
cd docker
./stop.sh
```

## Manual Commands

### Start container
```bash
cd docker
docker compose up -d
```

### Execute command in container
```bash
docker exec -it ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && <your command>"
```

### View GPU status
```bash
docker exec ros-humble-dev nvidia-smi
```

### Build workspace manually
```bash
docker exec -it ros-humble-dev bash -c "source /opt/ros/humble/setup.bash && cd /workspace/ros_bootcamp_ws && colcon build --symlink-install"
```

## Container Info
- **Image:** ros-humble-gpu:latest
- **Container Name:** ros-humble-dev
- **ROS Version:** Humble
- **GPU:** NVIDIA RTX 2080 Super with CUDA 12.5
- **Workspace:** /workspace/ros_bootcamp_ws

## Volumes
- Source code: Mounted from `../ros_bootcamp_ws/src`
- Build artifacts: Docker volumes (humble_build, humble_install, humble_log)
