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

### 2. 构建项目

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

### 服务调用示例

#### 连接设备
```bash
ros2 service call /io_left_gripper/connect std_srvs/srv/Trigger "{}"
```

#### 初始化夹爪
```bash
ros2 service call /io_left_gripper/initialize std_srvs/srv/Trigger "{}"
```

#### 设置位置
```bash
ros2 service call /io_left_gripper/command_position io_gripper_interfaces/srv/CommandPosition "{mode: 0, position_raw: 2742}"
```

#### 设置速度
```bash
ros2 service call /io_left_gripper/command_velocity io_gripper_interfaces/srv/CommandVelocity "{mode: 0, velocity_raw: 10}" 
```

#### 设置力矩限制
```bash
ros2 service call /io_left_gripper/set_effort_limit io_gripper_interfaces/srv/SetEffortLimit "{mode: 2, current_limit_ma: 3250, torque_limit_raw: 2000}"
```

#### 设置软限位
```bash
ros2 service call /io_left_gripper/set_soft_limit io_gripper_interfaces/srv/SetSoftLimit "{min_width_mm: 10.0, max_width_mm: 80.2}"
```

#### 使能/禁用力矩
```bash
ros2 service call /io_left_gripper/set_torque io_gripper_interfaces/srv/SetTorque "{enable: false}"
```

#### 抓取物体
```bash
ros2 service call /io_left_gripper/pick_object io_gripper_interfaces/srv/PickObject "{width_mm: 20.5, speed: 600, effort: 1000, timeout_ms: 1000}"
```

#### 紧急停止
```bash
ros2 service call /io_left_gripper/emergency_stop io_gripper_interfaces/srv/EmergencyStop "{release_torque: true}"
```

#### 获取状态
```bash
ros2 service call /io_left_gripper/get_status io_gripper_interfaces/srv/GetStatus "{}"
```

#### 扫描舵机 ID
```bash
ros2 service call /io_left_gripper/scan_ids io_gripper_interfaces/srv/ScanIds "{start_id: 1, end_id: 10}"
```

#### 修复配置(修复舵机ID)
```bash
ros2 service call /io_left_gripper/fix_config io_gripper_interfaces/srv/FixConfig "{servo_id: 2}"
```

#### 启动状态轮询
```bash
ros2 service call /io_gripper/start_polling io_gripper_interfaces/srv/StartPolling "{rate_hz: 100}"
```

### 开启轮询后获取轮询状态 once只获取一次
```bash
ros2 topic echo /io_gripper/status --once
```

### 断开连接
```bash
ros2 service call /io_gripper/disconnect std_srvs/srv/Trigger "{}"
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

修改配置文件后需要重新启动节点，或者调用 `fix_config` 服务后重新连接。

### Q: 子模块更新后编译失败？

尝试重新构建整个工作空间：
```bash
rm -rf build/ install/ log/
colcon build --symlink-install
```

## 使用 dockerfile 构建（x_64）
```bash
sudo docker build \
  --build-arg HTTP_PROXY=http://192.168.2.164:8082 \
  --build-arg HTTPS_PROXY=http://192.168.2.164:8082 \
  --build-arg NO_PROXY=localhost,127.0.0.1 \
  -f container/Containerfile_x64.Dockerfile \
  -t io_gripper_ros:latest .
```

## 运行节点
```bash
sudo docker run --rm -it \
 --net=host \
 --ipc=host\
  --privileged \
  -v /dev:/dev \
  -v ~/fpy/ROS2_SDK/ros2_ws:/root/workspace/ros2_ws \
  io_gripper_ros:latest \
  bash -c "cd /root/workspace/ros2_ws && \
           source /opt/ros/humble/setup.bash && \
           source install/setup.bash && \
           ros2 run io_gripper_ros io_gripper_node \
             --ros-args \
             -r __node:=left_gripper_node \
             -r __ns:=/io_left_gripper \
             -p auto_detect_port:=true \
             -p camera_serial:=G2026061 \
             -p config_name:=new_config.yaml"
```

