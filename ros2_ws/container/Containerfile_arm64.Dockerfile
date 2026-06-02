# -----------------------------
# Base image for ARM64 ROS2 Humble
# -----------------------------
ARG PLATFORM=linux/arm64
FROM --platform=$PLATFORM osrf/ros:humble-desktop-full AS deps

ENV DEBIAN_FRONTEND=noninteractive

ARG HTTP_PROXY
ARG HTTPS_PROXY
ARG NO_PROXY

ENV http_proxy=${HTTP_PROXY}
ENV https_proxy=${HTTPS_PROXY}
ENV HTTP_PROXY=${HTTP_PROXY}
ENV HTTPS_PROXY=${HTTPS_PROXY}
ENV no_proxy=${NO_PROXY}
ENV NO_PROXY=${NO_PROXY}

# --------------------------------------------
# Build Dependencies
# --------------------------------------------
RUN apt-get -o Acquire::http::Proxy="${HTTP_PROXY}" \
            -o Acquire::https::Proxy="${HTTPS_PROXY}" \
    update && apt-get -o Acquire::http::Proxy="${HTTP_PROXY}" \
                      -o Acquire::https::Proxy="${HTTPS_PROXY}" \
    install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    git-lfs \
    vim \
    tmux \
    curl \
    openssh-client \
    openssh-server \
    libserial-dev \
    libyaml-cpp-dev \
    libjsonrpccpp-dev \
    libjsonrpccpp-tools \
    libi2c-dev \
    python3-dev \
    libeigen3-dev \
    libasio-dev \
    liburdfdom-headers-dev \
    liburdfdom-dev \
    ros-humble-ament-cmake \
    ros-humble-rclcpp \
    ros-humble-ament-index-cpp \
    ros-humble-std-srvs \
    python3-colcon-common-extensions \
    ros-humble-pinocchio \
    ros-humble-foonathan-memory-vendor \
    ros-humble-eigenpy \
    ros-humble-hpp-fcl \
    ros-humble-xacro \
    ros-humble-rosbag2-storage-mcap \
    ros-humble-joint-state-publisher-gui \
    ros-humble-ros2-control \
    ros-humble-rosbag2-cpp \
    ros-humble-rosbag2-storage \
    libprotobuf-dev \
    libprotoc-dev \
    protobuf-compiler \
    libcurl4-openssl-dev \
    pybind11-dev \
    python3-pip \
    net-tools \
    iputils-ping \
    libzmq3-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN git lfs install

# --------------------------------------------
# Builder stage: compile from source
# --------------------------------------------
FROM deps AS builder

ARG HTTP_PROXY
ARG HTTPS_PROXY
ARG NO_PROXY

ENV http_proxy=${HTTP_PROXY}
ENV https_proxy=${HTTPS_PROXY}
ENV HTTP_PROXY=${HTTP_PROXY}
ENV HTTPS_PROXY=${HTTPS_PROXY}
ENV no_proxy=${NO_PROXY}
ENV NO_PROXY=${NO_PROXY}

# qpOASES v3.2.1
RUN git clone --branch releases/3.2.1 --depth 1 \
        https://github.com/coin-or/qpOASES.git /tmp/qpOASES && \
    cmake -S /tmp/qpOASES -B /tmp/qpOASES/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/qpOASES/build -j$(nproc) && \
    cmake --install /tmp/qpOASES/build && \
    rm -rf /tmp/qpOASES

# Casadi v3.6.5
RUN git clone --branch 3.6.5 --depth 1 \
        https://github.com/casadi/casadi.git /tmp/casadi && \
    cmake -S /tmp/casadi -B /tmp/casadi/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_BUILD_TYPE=Release \
        -DWITH_PYTHON=OFF \
        -DWITH_MATLAB=OFF \
        -DINSTALL_INTERNAL_HEADERS=ON && \
    cmake --build /tmp/casadi/build -j$(nproc) && \
    cmake --install /tmp/casadi/build && \
    rm -rf /tmp/casadi

# Fast-CDR v1.0.24
RUN git clone --branch v1.0.24 --depth 1 \
        https://github.com/eProsima/Fast-CDR.git /tmp/Fast-CDR && \
    cmake -S /tmp/Fast-CDR -B /tmp/Fast-CDR/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/Fast-CDR/build -j$(nproc) && \
    cmake --install /tmp/Fast-CDR/build && \
    rm -rf /tmp/Fast-CDR

# Fast-DDS v2.13.0
RUN git clone --branch 2.13.0 --depth 1 \
        https://github.com/eProsima/Fast-DDS.git /tmp/Fast-DDS && \
    cmake -S /tmp/Fast-DDS -B /tmp/Fast-DDS/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/opt/ros/humble \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/Fast-DDS/build -j$(nproc) && \
    cmake --install /tmp/Fast-DDS/build && \
    rm -rf /tmp/Fast-DDS

# --------------------------------------------
# Final stage
# --------------------------------------------
FROM deps

COPY --from=builder /usr/local /usr/local

RUN ldconfig

# --------------------------------------------
# ROS2 Workspace setup
# --------------------------------------------
WORKDIR /ros2_ws

COPY src/ /ros2_ws/src/

SHELL ["/bin/bash", "-c"]

RUN source /opt/ros/humble/setup.bash && \
    colcon --log-base log_arm64 build \
      --merge-install \
      --install-base install_arm64 \
      --build-base build_arm64 \
      --packages-select io_gripper_interfaces io_gripper_ros

RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "source /ros2_ws/install_arm64/setup.bash" >> /root/.bashrc

WORKDIR /root/workspace

ENV http_proxy=
ENV https_proxy=
ENV HTTP_PROXY=
ENV HTTPS_PROXY=


CMD ["/bin/bash"]