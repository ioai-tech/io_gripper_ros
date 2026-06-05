/*
 * @Author: 培岩 樊 fanpy@io-ai.tech
 * @Date: 2026-05-25 14:58:16
 * @LastEditors: 培岩 樊 fanpy@io-ai.tech
 * @LastEditTime: 2026-06-01 11:06:04
 * @FilePath: /ROS2_SDK/ros2_ws/src/io_gripper_ros/src/io_gripper_node.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "io_gripper_ros/io_gripper_node.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <chrono>
#include <fstream>
#include <functional>
#include <string>

#include "io_gripper.hpp"
#include "io_gripper_ros/gripper_port_resolver.hpp"
#include "std_srvs/srv/trigger.hpp"

namespace io::gripper {

IoGripperNode::IoGripperNode() : Node("io_gripper_node") {
  // 1. 读取 ROS2 参数
  auto_detect_port_ = this->declare_parameter<bool>("auto_detect_port", true);

  port_ = this->declare_parameter<std::string>("port", "/dev/ttyUSB0");

  camera_image_port_ = this->declare_parameter<std::string>("camera_image_port", "/dev/video0");

  camera_serial_ = this->declare_parameter<std::string>("camera_serial", "");

  std::string package_share_dir =
      ament_index_cpp::get_package_share_directory("io_gripper_ros");

  std::string config_name = this->declare_parameter<std::string>(
      "config_name", "gripper_config.yaml");

  config_file_path_ = package_share_dir + "/config/" + config_name;

  RCLCPP_INFO(this->get_logger(), "auto_detect_port: %s",
              auto_detect_port_ ? "true" : "false");
  if(!auto_detect_port_) {
    RCLCPP_INFO(this->get_logger(), "Default Use port: %s", port_.c_str());
    RCLCPP_INFO(this->get_logger(), "Default UseCameraImagePort: %s", camera_image_port_.c_str());\
  }
  RCLCPP_INFO(this->get_logger(), "Config Use camera_serial: %s", camera_serial_.c_str());
  RCLCPP_INFO(this->get_logger(), "Config config_file_path: %s",
              config_file_path_.c_str());

  connect_srv_ = this->create_service<std_srvs::srv::Trigger>(
      "connect", std::bind(&IoGripperNode::connectCallback, this,
                           std::placeholders::_1, std::placeholders::_2));

  initialize_srv_ = this->create_service<std_srvs::srv::Trigger>(
      "initialize", std::bind(&IoGripperNode::initializeCallback, this,
                              std::placeholders::_1, std::placeholders::_2));

  startpolling_srv_ =
      this->create_service<io_gripper_interfaces::srv::StartPolling>(
          "start_polling",
          std::bind(&IoGripperNode::startpollingCallback, this,
                    std::placeholders::_1, std::placeholders::_2));

  target_sub_ = this->create_subscription<
      io_gripper_interfaces::msg::GripperCommand>(  // 这里写的是绝对topic
                                                    // 相对topic，会跟随
                                                    // namespace
                                                    // 变化之后可以考虑更改
      "gripper_command", 10,
      std::bind(&IoGripperNode::targetCallback, this, std::placeholders::_1));
  status_pub_ =
      this->create_publisher<io_gripper_interfaces::msg::Status>("status", 10);
  get_status_srv_ = this->create_service<io_gripper_interfaces::srv::GetStatus>(
      "get_status", std::bind(&IoGripperNode::getStatusCallback, this,
                              std::placeholders::_1, std::placeholders::_2));

  scanids_srv_ = this->create_service<io_gripper_interfaces::srv::ScanIds>(
      "scan_ids", std::bind(&IoGripperNode::scanidsCallback, this,
                            std::placeholders::_1, std::placeholders::_2));

  settorque_srv_ = this->create_service<io_gripper_interfaces::srv::SetTorque>(
      "set_torque", std::bind(&IoGripperNode::settorqueCallback, this,
                              std::placeholders::_1, std::placeholders::_2));

  disconnect_srv_ = this->create_service<std_srvs::srv::Trigger>(
      "disconnect", std::bind(&IoGripperNode::disconnectCallback, this,
                              std::placeholders::_1, std::placeholders::_2));

  calibrate_srv_ = this->create_service<std_srvs::srv::Trigger>(
      "calibrate", std::bind(&IoGripperNode::calibrateCallback, this,
                             std::placeholders::_1, std::placeholders::_2));

  emergencystop_srv_ =
      this->create_service<io_gripper_interfaces::srv::EmergencyStop>(
          "emergency_stop",
          std::bind(&IoGripperNode::emergencystopCallback, this,
                    std::placeholders::_1, std::placeholders::_2));

  set_soft_limit_srv_ =
      this->create_service<io_gripper_interfaces::srv::SetSoftLimit>(
          "set_soft_limit",
          std::bind(&IoGripperNode::setSoftLimitCallback, this,
                    std::placeholders::_1, std::placeholders::_2));
  set_effort_limit_srv_ =
      this->create_service<io_gripper_interfaces::srv::SetEffortLimit>(
          "set_effort_limit",
          std::bind(&IoGripperNode::setEffortLimitCallback, this,
                    std::placeholders::_1, std::placeholders::_2));
  pick_object_srv_ =
      this->create_service<io_gripper_interfaces::srv::PickObject>(
          "pick_object",
          std::bind(&IoGripperNode::pickObjectCallback, this,
                    std::placeholders::_1, std::placeholders::_2));
  command_position_srv_ =
      this->create_service<io_gripper_interfaces::srv::CommandPosition>(
          "command_position",
          std::bind(&IoGripperNode::commandPositionCallback, this,
                    std::placeholders::_1, std::placeholders::_2));
  command_velocity_srv_ =
      this->create_service<io_gripper_interfaces::srv::CommandVelocity>(
          "command_velocity",
          std::bind(&IoGripperNode::commandVelocityCallback, this,
                    std::placeholders::_1, std::placeholders::_2));

  clear_fault_srv_ = this->create_service<std_srvs::srv::Trigger>(
      "clear_fault", std::bind(&IoGripperNode::clearFaultCallback, this,
                               std::placeholders::_1, std::placeholders::_2));

  fix_config_srv_ = this->create_service<io_gripper_interfaces::srv::FixConfig>(
      "fix_config", std::bind(&IoGripperNode::fixConfigCallback, this,
                              std::placeholders::_1, std::placeholders::_2));

  // camera_image_pub_ = this->create_publisher<sensor_msgs::msg::CompressedImage>(
  //     "camera_image", 10);
  rclcpp::SensorDataQoS qos;
  qos.keep_last(10); // 缓存最新 10 帧
  camera_image_pub_ =
    this->create_publisher<sensor_msgs::msg::CompressedImage>("camera_image", qos);

  stop_camera_srv_ = this->create_service<std_srvs::srv::Trigger>(
        "stop_camera",
        std::bind(&IoGripperNode::stopCameraCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
  
  get_camera_settings_srv_ =
    this->create_service<io_gripper_interfaces::srv::GetCameraSettings>(
        "get_camera_settings",
        std::bind(&IoGripperNode::getCameraSettingsCallback,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2));

  if (!createDriver()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to create GripperDriver: %s",
                 last_error_message_.c_str());
  } else {
    RCLCPP_INFO(this->get_logger(),
                "GripperDriver object created. Please call 'connect' service.");
  }
}

// 根据相机寻找串口
std::string IoGripperNode::resolvePort() {
  if (!auto_detect_port_) {
    RCLCPP_INFO(this->get_logger(),
                "Auto port detection disabled. Use configured port: %s",
                port_.c_str());
    return port_;
  }

  port_resolver_ = std::make_unique<GripperPortResolver>();

  try {
    port_resolver_->printAllMappings();

    std::string resolved_port =
        port_resolver_->resolveByCameraSerial(camera_serial_);

    RCLCPP_INFO(this->get_logger(),
                "Resolved gripper port: camera_serial=%s, port=%s",
                camera_serial_.c_str(), resolved_port.c_str());

    return resolved_port;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to resolve gripper port: %s",
                 e.what());
    return "";
  }
}

//根据相机序列号寻找图像接口
std::string IoGripperNode::resolveCameraImagePort() {
  if (!auto_detect_port_) {
    RCLCPP_INFO(this->get_logger(),
                "Auto port detection disabled. Use configured CameraImagePort: %s",
                camera_image_port_.c_str());
    return camera_image_port_;
  }

  port_resolver_ = std::make_unique<GripperPortResolver>();

  try {
    port_resolver_->printAllMappings();

    std::string resolved_port =
        port_resolver_->resolveCameraImageByCameraSerial(camera_serial_);

    RCLCPP_INFO(this->get_logger(),
                "Resolved CameraImagePort port: camera_serial=%s, port=%s",
                camera_serial_.c_str(), resolved_port.c_str());

    return resolved_port;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to resolve CameraImagePort port: %s",
                 e.what());
    return "";
  }
}

// 初始化夹爪对象
bool IoGripperNode::createDriver() {
  if (config_file_path_.empty()) {
    last_error_message_ = "config_file_path is empty.";
    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }
  if (!port_resolver_) {
    port_resolver_ = std::make_unique<GripperPortResolver>();
  }

  port_ = resolvePort();
  if (port_.empty()) {
    last_error_message_ = "Failed to resolve gripper port.";
    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  camera_image_port_ = resolveCameraImagePort();
  if (camera_image_port_.empty()) {
    last_error_message_ = "Failed to resolve CameraImagePort port.";
    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  RCLCPP_INFO(this->get_logger(), "Use gripper port: %s", port_.c_str());
  RCLCPP_INFO(this->get_logger(), "Use CameraImagePort port: %s", camera_image_port_.c_str());

  try {
    profile_ = port_resolver_->create_gripper_driver(config_file_path_);
  } catch (const std::exception& e) {
    last_error_message_ =
        std::string("Failed to load DeviceProfile from config file: ") +
        config_file_path_ + ", error: " + e.what();

    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  try {
    driver_ =
        std::make_unique<GripperDriver>(port_, profile_, config_file_path_);
        
  } catch (const std::exception& e) {
    last_error_message_ =
        std::string("Failed to create GripperDriver: ") + e.what();

    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  try {
    camera_ =
        std::make_unique<GripperCamera>(camera_image_port_, profile_);
  } catch (const std::exception& e) {
    last_error_message_ =
        std::string("Failed to create GripperCamera: ") + e.what();

    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  driver_created_ = true;
  driver_connected_ = false;
  driver_initialized_ = false;
  last_error_message_ = "Driver object created.";

  RCLCPP_INFO(this->get_logger(),
              "GripperDriver object created. port=%s, config=%s, servo_id=%d",
              port_.c_str(), config_file_path_.c_str(), profile_.servo_id);

  return true;
}

// 连接对象
bool IoGripperNode::connectDriver() {
  if (!driver_created_ || !driver_) {
    if (!createDriver()) {
      return false;
    }
  }

  if (driver_connected_) {
    last_error_message_ = "Driver is already connected.";
    RCLCPP_INFO(this->get_logger(), "%s", last_error_message_.c_str());
    return true;
  }

  if (!driver_->connect()) {
    last_error_message_ = "Failed to connect gripper driver on port: " + port_;

    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    driver_connected_ = false;
    driver_initialized_ = false;
    return false;
  }

  driver_connected_ = true;
  driver_initialized_ = false;
  last_error_message_ = "Driver connected successfully.";
  RCLCPP_INFO(this->get_logger(), "%s", "Please call initialize before publish camera image.");
  float fps = profile_.fps;

  if (fps <= 0.0) {
    fps = 30.0;
  }
  RCLCPP_INFO(this->get_logger(), "Use fps: %.2f", fps);

  auto period_ms = static_cast<int>(1000.0 / fps);

  camera_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&IoGripperNode::publishCameraImage, this));

  RCLCPP_INFO(this->get_logger(),
              "GripperDriver connected successfully. port=%s", port_.c_str());

  return true;
}

// 初始化夹爪
bool IoGripperNode::initializeGripper() {
  if (!driver_ || !driver_created_) {
    last_error_message_ =
        "Driver object is not created. Please call connect first.";
    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  if (!driver_connected_) {
    last_error_message_ = "Driver is not connected. Please call connect first.";
    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    return false;
  }

  if (driver_initialized_) {
    last_error_message_ = "Driver is already initialized.";
    RCLCPP_INFO(this->get_logger(), "%s", last_error_message_.c_str());
    return true;
  }

  const uint8_t servo_id = static_cast<uint8_t>(profile_.servo_id);

  if (!driver_->initialize({servo_id})) {
    last_error_message_ =
        "Failed to initialize servo id: " + std::to_string(profile_.servo_id);

    RCLCPP_ERROR(this->get_logger(), "%s", last_error_message_.c_str());
    driver_initialized_ = false;
    return false;
  }

  driver_initialized_ = true;
  last_error_message_ = "Driver initialized successfully.";

  RCLCPP_INFO(this->get_logger(),
              "Gripper driver initialized. port=%s, config=%s, servo_id=%d",
              port_.c_str(), config_file_path_.c_str(), profile_.servo_id);

  return true;
}

bool IoGripperNode::isDriverReady() const {
  return driver_ && driver_connected_ && driver_initialized_;
}

void IoGripperNode::connectCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
  (void)request;

  bool ok = connectDriver();

  response->success = ok;
  response->message = ok ? "Connect success." : last_error_message_;
}

void IoGripperNode::initializeCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
  (void)request;

  bool ok = initializeGripper();

  response->success = ok;
  response->message = ok ? "Initialize success." : last_error_message_;
}

void IoGripperNode::startpollingCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::StartPolling::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::StartPolling::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  double hz = static_cast<double>(request->rate_hz);

  if (hz <= 0.0) {
    response->success = false;
    response->message = "rate_hz must be greater than 0.";
    return;
  }

  // 1. 开启 SDK 内部轮询
  driver_->startPolling(hz);

  // 2. 根据同一个频率设置 ROS 状态发布 timer
  auto period_ms = static_cast<int64_t>(1000.0 / hz);

  if (period_ms < 1) {
    period_ms = 1;
  }

  if (timer_) {
    timer_->cancel();
    timer_.reset();
  }

  timer_ =
      this->create_wall_timer(std::chrono::milliseconds(period_ms),
                              std::bind(&IoGripperNode::publishState, this));

  response->success = true;
  response->message = "Start polling success. Status publish rate is set to " +
                      std::to_string(hz) + " Hz.";
}

void IoGripperNode::targetCallback(
    const io_gripper_interfaces::msg::GripperCommand::SharedPtr msg) {
  GripperCommand cmd{};
  if (!isDriverReady()) {
    RCLCPP_ERROR(this->get_logger(),
                 "Driver is not ready, ignore gripper command.");
    return;
  }

  if (msg->mode == io_gripper_interfaces::msg::GripperCommand::MODE_WIDTH_MM) {
    cmd.use_width_mm = true;
    cmd.width_mm = msg->width_mm;

    cmd.use_normalized_opening = false;
    cmd.normalized_opening = 0.0f;
    cmd.max_effort = msg->max_effort;
    cmd.speed = msg->speed;
    driver_->commandGripper(cmd);
  } else if (msg->mode ==
             io_gripper_interfaces::msg::GripperCommand::MODE_NORMALIZED) {
    cmd.use_width_mm = false;
    cmd.width_mm = 0.0f;

    cmd.use_normalized_opening = true;
    cmd.normalized_opening = msg->normalized_opening;
    cmd.max_effort = msg->max_effort;
    cmd.speed = msg->speed;
    driver_->commandGripper(cmd);
  } else {
    RCLCPP_ERROR(this->get_logger(), "Invalid gripper command mode: %u",
                 msg->mode);
    return;
  }
}

void IoGripperNode::publishState() {
  io_gripper_interfaces::msg::Status status;

  status.header.stamp = this->now();
  status.header.frame_id = "gripper";

  status.servo_id = static_cast<uint8_t>(profile_.servo_id);
  if (!isDriverReady()) {
    status.driver_state = static_cast<uint8_t>(driver_->state());
    status.communication_ok = false;
    status.message = "driver object is not created or connected or initialized";
    status_pub_->publish(status);
    return;
  }

  try {
    // 从缓存中获取，前提是要开启轮询
    GripperState state =
        driver_->getCachedState(static_cast<uint8_t>(profile_.servo_id));
    status.driver_state = static_cast<uint8_t>(driver_->state());
    status.communication_ok = true;
    status.message = "ok";

    status.position_raw = state.position_raw;
    status.position_rad = state.position_rad;

    status.velocity_raw = state.velocity_raw;
    status.velocity_rad_s = state.velocity_rad_s;

    status.has_load = true;
    status.load_raw = state.load_raw;

    status.voltage_v = state.voltage_V;
    status.temperature_c = state.temperature_C;

  } catch (const std::exception& e) {
    status.communication_ok = false;
    status.message = std::string("readState failed: ") + e.what();
  }

  status_pub_->publish(status);
}

void IoGripperNode::getStatusCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::GetStatus::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::GetStatus::Response> response) {
  (void)request;
  response->header.stamp = this->now();
  response->header.frame_id = "gripper";

  response->servo_id = static_cast<uint8_t>(profile_.servo_id);
  if (!isDriverReady()) {
    response->driver_state = static_cast<uint8_t>(driver_->state());
    response->communication_ok = false;
    response->message =
        "Driver object is not created or connected or initialized";
    return;
  }

  try {
    GripperState state =
        driver_->readState(static_cast<uint8_t>(profile_.servo_id));

    response->driver_state = static_cast<uint8_t>(driver_->state());
    response->communication_ok = true;
    response->message = "ok";

    response->position_raw = state.position_raw;
    response->position_rad = state.position_rad;

    response->velocity_raw = state.velocity_raw;
    response->velocity_rad_s = state.velocity_rad_s;

    response->has_load = true;
    response->load_raw = state.load_raw;

    response->voltage_v = state.voltage_V;
    response->temperature_c = state.temperature_C;

  } catch (const std::exception& e) {
    response->communication_ok = false;
    response->message = std::string("get status failed: ") + e.what();
  }
}

// 扫描ID的回调函数
void IoGripperNode::scanidsCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::ScanIds::Request> request,
    std::shared_ptr<io_gripper_interfaces::srv::ScanIds::Response> response) {
  if (!driver_ || !driver_connected_) {
    response->success = false; stop_camera_srv_ = this->create_service<std_srvs::srv::Trigger>(
        "/io_left_gripper/stop_camera",
        std::bind(&IoGripperNode::stopCameraCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
    response->message = "Driver is not created or not connected.";
    return;
  }
  std::vector<uint8_t> res_ids =
      driver_->scanIds(request->start_id, request->end_id);
  if (res_ids.empty()) {
    response->success = false;
    response->message = "No find id online";
    response->ids = res_ids;
    return;
  }
  response->success = true;
  response->message = "Success find ids";
  response->ids = res_ids;
  return;
}

void IoGripperNode::settorqueCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::SetTorque::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::SetTorque::Response> response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  if (request->enable == true) {
    bool res_ok = driver_->enableTorque(profile_.servo_id);
    response->success = res_ok;
    response->message = res_ok ? "set torque success" : "set torque failed";
  } else {
    bool res_ok = driver_->disableTorque(profile_.servo_id);
    response->success = res_ok;
    response->message =
        res_ok ? "release torque success" : "release torque failed";
  }
}

void IoGripperNode::disconnectCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
  (void)request;

  if (!driver_connected_) {
    response->success = true;
    response->message = "Driver is already not connected.";
    return;
  }

  try {
    driver_->stopPolling();
    driver_->disconnect();

    driver_connected_ = false;
    driver_initialized_ = false;

    response->success = true;
    response->message = "Gripper disconnected successfully.";
  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Disconnect failed: ") + e.what();
  }
}

void IoGripperNode::calibrateCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
  (void)request;

  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  // 2. 防止重复调用标定
  //    calibrate() 是长时间动作，内部还会运动夹爪和写入 YAML。
  bool expected = false;
  if (!calibrating_.compare_exchange_strong(expected, true)) {
    response->success = false;
    response->message = "Calibration is already running.";
    return;
  }

  try {
    // 3. 具体状态判断交给 SDK 内部完成
    //    SDK 已经判断 DISCONNECTED / MOVING / FAULT / ESTOP。
    const bool ok = driver_->calibrate();

    response->success = ok;
    response->message =
        ok ? "Gripper calibration succeeded."
           : "Gripper calibration failed. Check SDK output for details.";
  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Calibration exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Calibration failed with unknown exception.";
  }

  calibrating_.store(false);
}

void IoGripperNode::emergencystopCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::EmergencyStop::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::EmergencyStop::Response>
        response) {
  if (!driver_) {
    response->success = false;
    response->message = "Driver object is not created.";
    return;
  }

  if (!driver_connected_) {
    response->success = false;
    response->message = "Driver is not connected.";
    return;
  }

  try {
    bool res_release = request->release_torque;
    driver_->emergencyStop(res_release);
    driver_initialized_ = false;
    response->success = true;
    response->message = "Emergency stop success. Torque released.";
  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Emergency stop exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Emergency stop failed with unknown exception.";
  }
}

void IoGripperNode::setSoftLimitCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::SetSoftLimit::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::SetSoftLimit::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver not ready: missing, disconnected or not initialized.";
    return;
  }
  try {
    bool res_set =
        driver_->setSoftLimits(request->min_width_mm, request->max_width_mm);

    response->success = res_set;
    response->message = res_set ? "Soft limits set successfully."
                                : "Failed to set soft limits.";

  } catch (const std::exception& e) {
    response->success = false;
    response->message =
        std::string("Exception setting soft limits: ") + e.what();
  }
}
void IoGripperNode::setEffortLimitCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::SetEffortLimit::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::SetEffortLimit::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: missing, disconnected, or not initialized.";
    return;
  }

  try {
    bool ok = false;

    if (request->mode ==
        io_gripper_interfaces::srv::SetEffortLimit::Request::MODE_CURRENT_MA) {
      // 使用电流限制
      ok = driver_->setEffortLimit(profile_.servo_id, request->current_limit_ma,
                                   std::nullopt);

    } else if (request->mode == io_gripper_interfaces::srv::SetEffortLimit::
                                    Request::MODE_TORQUE_RAW) {
      // 使用脉冲力矩限制
      ok = driver_->setEffortLimit(profile_.servo_id, std::nullopt,
                                   request->torque_limit_raw);
    } else if (request->mode == io_gripper_interfaces::srv::SetEffortLimit::
                                    Request::MODE_ALLSELECT) {
      // 两者全部使用
      ok = driver_->setEffortLimit(profile_.servo_id, request->current_limit_ma,
                                   request->torque_limit_raw);
    } else {
      response->success = false;
      response->message = "Invalid mode value.";
      return;
    }

    response->success = ok;
    response->message = ok ? "limit set successfully." : "Failed to set limit.";

  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Failed to set effort limit: unknown exception.";
  }
}

void IoGripperNode::pickObjectCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::PickObject::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::PickObject::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: missing, disconnected, or not initialized.";
    return;
  }
  try {
    bool ok = driver_->pickObject(profile_.servo_id, request->width_mm,
                                  request->speed, request->effort,
                                  request->timeout_ms);
    response->success = ok;
    response->message = ok ? "Pick object succeeded." : "Pick object failed.";

  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Pick object exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Pick object failed with unknown exception.";
  }
}

void IoGripperNode::commandPositionCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::CommandPosition::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::CommandPosition::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  try {
    bool ok = false;

    if (request->mode ==
        io_gripper_interfaces::srv::CommandPosition::Request::MODE_RAW) {
      const uint16_t target_raw = request->position_raw;

      ok =
          driver_->commandPosition(profile_.servo_id, target_raw, std::nullopt);

      response->success = ok;
      response->message =
          ok ? "Command position raw success." : "Command position raw failed.";
      return;

    } else if (request->mode ==
               io_gripper_interfaces::srv::CommandPosition::Request::MODE_RAD) {
      const float target_rad = request->angle_rad;

      ok =
          driver_->commandPosition(profile_.servo_id, std::nullopt, target_rad);

      response->success = ok;
      response->message =
          ok ? "Command position rad success." : "Command position rad failed.";
      return;

    } else {
      response->success = false;
      response->message = "Invalid command position mode.";
      return;
    }

  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Command position exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Command position failed with unknown exception.";
  }
}

void IoGripperNode::commandVelocityCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::CommandVelocity::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::CommandVelocity::Response>
        response) {
  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  try {
    bool ok = false;

    if (request->mode ==
        io_gripper_interfaces::srv::CommandVelocity::Request::MODE_RAW) {
      ok = driver_->commandVelocity(profile_.servo_id, request->velocity_raw,
                                    std::nullopt, std::nullopt);

      response->success = ok;
      response->message = ok ? "Command velocity raw set success."
                             : "Command velocity raw set failed.";
      return;

    } else if (request->mode == io_gripper_interfaces::srv::CommandVelocity::
                                    Request::MODE_RAD_S) {
      ok = driver_->commandVelocity(profile_.servo_id, std::nullopt,
                                    request->velocity_rad_s, std::nullopt);

      response->success = ok;
      response->message = ok ? "Command velocity rad/s set success."
                             : "Command velocity rad/s set failed.";
      return;

    } else {
      response->success = false;
      response->message = "Invalid command velocity mode.";
      return;
    }

  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Command velocity exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Command velocity failed with unknown exception.";
  }
}

void IoGripperNode::clearFaultCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
  (void)request;

  if (!isDriverReady()) {
    response->success = false;
    response->message =
        "Driver is not ready: object missing, not connected, or not "
        "initialized.";
    return;
  }

  try {
    bool ok = driver_->clearFault();

    if (!ok) {
      response->success = false;
      response->message = "Clear fault failed.";
      return;
    }

    response->success = true;
    response->message = "Clear fault success.";
  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Clear fault exception: ") + e.what();
  } catch (...) {
    response->success = false;
    response->message = "Clear fault failed with unknown exception.";
  }
}

void IoGripperNode::fixConfigCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::FixConfig::Request>
        request,
    std::shared_ptr<io_gripper_interfaces::srv::FixConfig::Response> response) {
  if (config_file_path_.empty()) {
    response->success = false;
    response->message = "config_file_path is empty.";
    return;
  }

  try {
    YAML::Node config = YAML::LoadFile(config_file_path_);

    config["DeviceProfile"]["device_info"]["servo_id"] =
        static_cast<int>(request->servo_id);

    std::ofstream fout(config_file_path_);
    if (!fout.is_open()) {
      response->success = false;
      response->message =
          "Failed to open config file for writing: " + config_file_path_;
      return;
    }

    fout << config;

    fout.close();

    // 修改profile中的id
    profile_.servo_id = request->servo_id;
    if (driver_) {
      driver_->disconnect();  // 断开连接
      driver_.reset();        // 销毁 driver 对象
    }
    driver_created_ = false;
    driver_connected_ = false;
    driver_initialized_ = false;
    RCLCPP_INFO(this->get_logger(),
                "fix completed disconnect, call connect again");
    // driver_initialized_ = false;
    response->success = true;
    response->message =
        "servo_id updated to " + std::to_string(request->servo_id);

    RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());

  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Failed to update servo_id: ") + e.what();
  }
}


void IoGripperNode::publishCameraImage() {
  if (!isDriverReady()) {
    return;
  }

  try {
    GripperCompressedImage image{};

    if (!camera_->captureCompressedImage(image)) {
      RCLCPP_WARN(this->get_logger(), "Failed to capture camera image.");
      return;
    }

    sensor_msgs::msg::CompressedImage msg;

    msg.header.stamp = this->now();
    if (!camera_serial_.empty()) {
      msg.header.frame_id = "gripper_camera: " + camera_serial_;
    } else {
      msg.header.frame_id = "gripper_camera";
    }
    msg.format = image.format;       // jpeg
    msg.data = std::move(image.data);

    camera_image_pub_->publish(msg);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to publish camera image: %s",
                 e.what());
  }
}


void IoGripperNode::stopCameraCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
    (void)request;

    if (camera_timer_ && camera_timer_->is_ready()) {
        camera_timer_->cancel();
        response->success = true;
        response->message = "Camera publisher stopped.";
        RCLCPP_INFO(this->get_logger(), "Camera image publishing stopped by service call.");
    } else {
        response->success = false;
        response->message = "Camera timer not running or already stopped.";
        RCLCPP_WARN(this->get_logger(), "Camera timer already stopped.");
    }
}

void IoGripperNode::getCameraSettingsCallback(
    const std::shared_ptr<io_gripper_interfaces::srv::GetCameraSettings::Request> request,
    std::shared_ptr<io_gripper_interfaces::srv::GetCameraSettings::Response> response) {
  (void)request;

  if (!camera_) {
    response->success = false;
    response->message = "Camera object is not created.";
    response->width = 0.0;
    response->height = 0.0;
    response->fps = 0.0;
    return;
  }


  try {
    CameraSettings s = camera_->getCameraSettings(camera_image_port_);

    response->success = true;
    response->message = "Get camera settings successfully.";
    response->width = s.width;
    response->height = s.height;
    response->fps = s.fps;

    RCLCPP_INFO(this->get_logger(),
                "Camera settings: width=%.2f, height=%.2f, fps=%.2f",
                s.width, s.height, s.fps);
  } catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("Failed to get camera settings: ") + e.what();
    response->width = 0.0;
    response->height = 0.0;
    response->fps = 0.0;
  }
}

}  // namespace io::gripper