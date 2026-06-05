# io_gripper_ros

ROS2 夹爪控制节点，基于 `io_gripper_sdk` 实现对夹爪设备的控制和状态管理。

## 项目结构

```
ros2_ws/src/
├── io_gripper_interfaces/    # ROS2 自定义消息和服务定义
├── io_gripper_ros/           # ROS2 节点实现（主包）
└── io_gripper_sdk/           # 夹爪 SDK 子模块
```

## 功能特性

- ✅ 夹爪位置控制
- ✅ 夹爪速度控制
- ✅ 力矩控制与限制
- ✅ 紧急停止功能
- ✅ 软限位设置
- ✅ 舵机 ID 扫描
- ✅ 配置文件管理
- ✅ 实时状态轮询

## 安装说明

### 1. 克隆项目（包含子模块）

```bash
git clone --recurse-submodules <仓库地址>
cd ROS2_SDK/ros2_ws
```

### 2. 构建项目(在ros2_ws下)

```bash
colcon build --packages-select io_gripper_interfaces io_gripper_ros
source install/setup.bash
```

### 3. 更新子模块（如果需要）

```bash
git submodule update --init --recursive
```

## 使用方法

### 启动节点

```bash
ros2 run io_gripper_ros io_gripper_node --ros-args -r __node:=left_gripper_node -r __ns:=/io_left_gripper -p auto_detect_port:=true -p camera_serial:=G2026061 -p config_name:=new_config.yaml
```
### 命令说明
  -`__node:=left_gripper_node` 节点名称 不同节点对应不同的节点名称
  -`__ns:=/io_left_gripper` 命名空间 不同的夹爪节点必须用不同的命名空间，同时各自对应的 topic和service 也要修改
  -`auto_detect_port:=true` 是否自动检测串口端口 默认true。 如果选择false 则需要手动指定串口设备名称和相机端口（或者使用默认值），同时不必传入`camera_serial`
  -`port:=/dev/ttyUSB0` 串口设备名称，如果上面取消了自动检测端口，需要手动指定串口设备名称，或者不传入参数，选择使用默认值 ‘/dev/ttyUSB0’
  -`camera_image_port:=/dev/video0` 相机端口，如果上面取消了自动检测端口，需要手动指定串口设备名称，或者不传入参数，选择使用默认值 ‘/dev/video0’
  -`camera_serial:=""` 夹爪相机对应的序列号，如果`auto_detect_port:=true`，则必须传入该参数来控制对应的夹爪。如果`auto_detect_port:=false`，则可以不传入该参数，使用默认值。
  -`config_name:=new_config.yaml` 配置文件名称

### 服务调用示例

#### 连接设备
```bash
ros2 service call /io_left_gripper/connect std_srvs/srv/Trigger "{}"
```

#### 初始化夹爪同时开始发布相机图像话题
```bash
ros2 service call /io_left_gripper/initialize std_srvs/srv/Trigger "{}"
```
### 标定 (只需要标定一次，前提是配对应的配置文件不变)
```bash
ros2 service call /io_left_gripper/calibrate std_srvs/srv/Trigger "{}"
```
### 夹爪语义控制
```bash
ros2 topic pub --once /io_left_gripper/gripper_command io_gripper_interfaces/msg/GripperCommand "{mode: 0, width_mm: 120, max_effort: 0.5, speed: 0.5}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `mode` | uint8 | 控制模式：`0` = 毫米模式（width_mm），`1` = 归一化模式（normalized_opening） |
| `width_mm` | float32 | 目标开口宽度（毫米），仅在 mode=0 时有效，范围由配置文件中的范围或者设置的软限位决定(不能低于软限位的最小值) |
| `normalized_opening` | float32 | 归一化开口度（0.0-1.0），仅在 mode=1 时有效，0.0 表示完全闭合，1.0 表示完全打开 |
| `max_effort` | float32 | 最大作用力（0.0-1.0），0.0 表示无限制，1.0 表示最大力 |
| `speed` | float32 | 运动速度（0.0-1.0），0.0 表示最慢，1.0 表示最快 |

**使用示例**：

1. **毫米模式** - 设置开口宽度为 50mm：
```bash
ros2 topic pub --once /io_left_gripper/gripper_command io_gripper_interfaces/msg/GripperCommand "{mode: 0, width_mm: 50.0, max_effort: 0.8, speed: 0.5}"
```

2. **归一化模式** - 设置开口度为 50%：
```bash
ros2 topic pub --once /io_left_gripper/gripper_command io_gripper_interfaces/msg/GripperCommand "{mode: 1, normalized_opening: 0.5, max_effort: 0.6, speed: 0.3}"
```

3. **完全打开**：
```bash
ros2 topic pub --once /io_left_gripper/gripper_command io_gripper_interfaces/msg/GripperCommand "{mode: 1, normalized_opening: 1.0, max_effort: 0.5, speed: 1.0}"
```

4. **完全闭合**：
```bash
ros2 topic pub --once /io_left_gripper/gripper_command io_gripper_interfaces/msg/GripperCommand "{mode: 1, normalized_opening: 0.0, max_effort: 1.0, speed: 0.5}"
```

#### 设置位置
```bash
ros2 service call /io_left_gripper/command_position io_gripper_interfaces/srv/CommandPosition "{mode: 0, position_raw: 2742}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `mode` | uint8 | 位置模式：`0` = 原始位置值（position_raw），`1` = 弧度（angle_rad） |
| `position_raw` | uint16 | 原始位置值（在限位范围内），仅在 mode=0 时有效 |
| `angle_rad` | float32 | 角度（弧度），仅在 mode=1 时有效 |

#### 设置速度
```bash
ros2 service call /io_left_gripper/command_velocity io_gripper_interfaces/srv/CommandVelocity "{mode: 0, velocity_raw: 10}" 
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `mode` | uint8 | 速度模式：`0` = 原始速度值（velocity_raw），`1` = 弧度/秒（velocity_rad_s） |
| `velocity_raw` | uint16 | 原始速度值（0-3000），仅在 mode=0 时有效，最大速度在配置文件中设置 |
| `velocity_rad_s` | float32 | 角速度（弧度/秒），仅在 mode=1 时有效 |

#### 设置力矩限制
```bash
ros2 service call /io_left_gripper/set_effort_limit io_gripper_interfaces/srv/SetEffortLimit "{mode: 2, current_limit_ma: 3250, torque_limit_raw: 2000}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `mode` | uint8 | 设置模式：`0` = 仅设置电流限制，`1` = 仅设置力矩限制，`2` = 同时设置两者 |
| `current_limit_ma` | float32 | 电流限制（毫安），默认范围 0-3250 mA |
| `torque_limit_raw` | uint16 | 力矩限制原始值（0-1000） |

#### 设置软限位
```bash
ros2 service call /io_left_gripper/set_soft_limit io_gripper_interfaces/srv/SetSoftLimit "{min_width_mm: 10.0, max_width_mm: 80.2}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `min_width_mm` | float32 | 最小开口宽度（毫米），夹爪无法闭合小于此值 |
| `max_width_mm` | float32 | 最大开口宽度（毫米），夹爪无法张开大于此值 |

#### 使能/禁用力矩
```bash
ros2 service call /io_left_gripper/set_torque io_gripper_interfaces/srv/SetTorque "{enable: false}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `enable` | bool | `true` = 使能力矩，`false` = 禁用力矩（夹爪释放） |

#### 抓取物体
```bash
ros2 service call /io_left_gripper/pick_object io_gripper_interfaces/srv/PickObject "{width_mm: 20.5, speed: 600, effort: 1000, timeout_ms: 1000}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `width_mm` | float32 | 目标抓取宽度（毫米） |
| `speed` | float32 | 抓取速度（原始值，0-3000） |
| `effort` | float32 | 抓取力（原始值，0-1000） |
| `timeout_ms` | int32 | 超时时间（毫秒），超时后停止抓取 |

#### 紧急停止 通过重新初始恢复
```bash
ros2 service call /io_left_gripper/emergency_stop io_gripper_interfaces/srv/EmergencyStop "{release_torque: true}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `release_torque` | bool | `true` = 紧急停止并释放力矩，`false` = 紧急停止但保持力矩 |

#### 获取状态
```bash
ros2 service call /io_left_gripper/get_status io_gripper_interfaces/srv/GetStatus "{}"
```

#### 扫描舵机 ID
```bash
ros2 service call /io_left_gripper/scan_ids io_gripper_interfaces/srv/ScanIds "{start_id: 1, end_id: 10}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `start_id` | uint8 | 扫描起始 ID（范围：0-253） |
| `end_id` | uint8 | 扫描结束 ID（范围：0-253），必须大于等于 start_id |

#### 修复配置(修复舵机ID)
```bash
ros2 service call /io_left_gripper/fix_config io_gripper_interfaces/srv/FixConfig "{servo_id: 2}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `servo_id` | uint8 | 目标舵机 ID（范围：0-253），将配置文件中的 servo_id 修改为此值 |

#### 启动状态轮询
```bash
ros2 service call /io_left_gripper/start_polling io_gripper_interfaces/srv/StartPolling "{rate_hz: 100}"
```

**参数说明**：

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `rate_hz` | float32 | 轮询频率（赫兹），建议范围 10-120 Hz |

### 开启轮询后获取轮询状态 once只获取一次
```bash
ros2 topic echo /io_left_gripper/status --once
```

### 获取相机图像
```bash
ros2 topic echo /io_left_gripper/camera_image --once
```

### 获取相机图像设置
```bash
ros2 service call /io_left_gripper/get_camera_settings io_gripper_interfaces/srv/GetCameraSettings "{}"
```

#### 停止发布相机图像话题
```bash
ros2 service call /io_left_gripper/stop_camera std_srvs/srv/Trigger "{}"
```
### 清理状态  在超过安全限制之后会进入故障状态，通过这里清除但不能清除急停状态
```bash
ros2 service call /io_left_gripper/clear_status std_srvs/srv/Trigger "{}"
```

### 断开连接
```bash
ros2 service call /io_left_gripper/disconnect std_srvs/srv/Trigger "{}"
```

## 话题发布

| 话题名               | 消息类型                           | 描述                                 |
| -------------------- | ---------------------------------- | ------------------------------------ |
| `/io_gripper/status` | `io_gripper_interfaces/msg/Status` | 夹爪状态（位置、速度、力矩、状态码） |

## 参数配置

| 参数名             | 类型   | 默认值                       | 描述         |
| ------------------ | ------ | ---------------------------- | ------------ |
| `port`             | string | `/dev/ttyUSB0`               | 串口设备路径 |
| `baud_rate`        | int    | 115200                       | 波特率       |
| `config_file_path` | string | `config/gripper_config.yaml` | 配置文件路径 |

## 配置文件格式

```yaml
DeviceProfile:
  device_info:
    model_name: Feetech_DeviceProfile
    servo_id: 1
    recommended_baudrate: 115200
  capabilities:
    supports_sync_write: true
    supports_sync_read: false
    supports_present_current: true
  safety_limits:
    min_voltage_v: 4.5
    max_voltage_v: 25.4
    max_temperature_c: 70.0
    max_servo_velocity: 3000
    start_power: 8
    release_torque_on_disconnect: true
  calibration:
    calib_max_position_raw: 2742
    calib_min_position_raw: 2293
    calib_max_width_mm: 100.0
    calib_min_width_mm: 0.0
  camera:
    width: 640
    height: 480
    fps: 30.0
    jpeg_quality: 80
```

## 状态码说明

| 状态码 | 描述     |
| ------ | -------- |
| 0      | 未初始化 |
| 1      | 已连接   |
| 2      | 就绪     |
| 3      | 运行中   |
| 4      | 故障     |

## 子模块说明

`io_gripper_sdk` 是一个独立的 git 子模块，提供夹爪硬件的底层驱动支持。

```bash
# 添加子模块
git submodule add git@git.io-ai.tech:io_sensexperience/io_gripper_sdk.git ros2_ws/src/io_gripper_sdk

# 更新子模块
git submodule update --remote
```

## 许可证

MIT License

## 开发说明

### 目录结构

```
io_gripper_ros/
├── config/              # 配置文件
├── include/io_gripper_ros/  # 头文件
│   ├── gripper_port_resolver.hpp  # 端口解析器
│   └── io_gripper_node.hpp       # 节点类定义
├── src/                 # 源文件
│   ├── gripper_port_resolver.cpp
│   ├── io_gripper_node.cpp
│   └── main.cpp
├── CMakeLists.txt
├── package.xml
└── LICENSE
```

## 常见问题

### Q: 无法连接设备？

确保串口设备权限正确：
```bash
sudo chmod 666 端口+
```

### Q: 配置文件修改后不生效？

通过`fix_config`服务修改配置文件后需要重新连接。

### Q: 子模块更新后编译失败？

尝试重新构建整个工作空间：
```bash
rm -rf build/ install/ log/
colcon build --symlink-install
```

## 使用 dockerfile 构建（x_64）
```bash
sudo docker build \
  --build-arg HTTP_PROXY=    \
  --build-arg HTTPS_PROXY=   \
  --build-arg NO_PROXY=localhost,127.0.0.1 \
  -f container/Containerfile_x64.Dockerfile \
  -t io_gripper_ros:latest .
```

## 运行节点
```bash
x64：
sudo docker run --rm -it \
 --net=host \
 --ipc=host\
  --privileged \
  -v /dev:/dev \
  io_gripper_ros:latest \
  bash -c "cd /ros2_ws && \
           source /opt/ros/humble/setup.bash && \
           source install_x64/setup.bash && \
           ros2 run io_gripper_ros io_gripper_node \
             --ros-args \
             -r __node:=left_gripper_node \
             -r __ns:=/io_left_gripper \
             -p auto_detect_port:=true \
             -p camera_serial:=G2026061 \
             -p config_name:=new_config.yaml"
```

``` bash
arm64版本：
sudo docker run --rm -it \
 --net=host \
 --ipc=host\
  --privileged \
  -v /dev:/dev \
  io_gripper_ros:latest \
  bash -c "cd /ros2_ws && \
           source /opt/ros/humble/setup.bash && \
           source install_arm64/setup.bash && \
           ros2 run io_gripper_ros io_gripper_node \
             --ros-args \
             -r __node:=left_gripper_node \
             -r __ns:=/io_left_gripper \
             -p auto_detect_port:=true \
             -p camera_serial:=G2026061 \
             -p config_name:=new_config.yaml"
```
