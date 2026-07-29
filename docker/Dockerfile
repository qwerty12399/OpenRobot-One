FROM osrf/ros:humble-desktop-full

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3-colcon-common-extensions \
        python3-rosdep \
        ros-humble-gazebo-ros-pkgs \
        ros-humble-joint-state-publisher-gui \
        ros-humble-navigation2 \
        ros-humble-nav2-bringup \
        ros-humble-slam-toolbox \
        ros-humble-teleop-twist-keyboard \
        ros-humble-tf2-tools \
        ros-humble-xacro \
    && rosdep update \
    && rm -rf /var/lib/apt/lists/*

COPY docker/entrypoint.sh /openrobot_entrypoint.sh
RUN chmod +x /openrobot_entrypoint.sh

WORKDIR /workspace

ENTRYPOINT ["/openrobot_entrypoint.sh"]
CMD ["bash"]
