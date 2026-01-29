# ROS Humble with NVIDIA GPU Support
FROM osrf/ros:humble-desktop-full

# Setup NVIDIA Container Toolkit
ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility

# Install additional dependencies
RUN apt-get update && apt-get install -y \
    python3-pip \
    python3-colcon-common-extensions \
    python3-rosdep \
    git \
    vim \
    nano \
    wget \
    curl \
    build-essential \
    cmake \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Install common Python packages for vision/ML workloads
# Note: Install additional packages as needed for your specific use case
RUN pip3 install --no-cache-dir --break-system-packages \
    numpy \
    opencv-python-headless || true

# Setup user to match host user (avoid permission issues)
ARG USERNAME=ros
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

# Setup workspace
RUN mkdir -p /workspace/ros_bootcamp_ws/src
WORKDIR /workspace/ros_bootcamp_ws

# Copy entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

USER $USERNAME
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
