/*
 * @Author: 培岩 樊 fanpy@io-ai.tech
 * @Date: 2026-05-25 14:58:16
 * @LastEditors: 培岩 樊 fanpy@io-ai.tech
 * @LastEditTime: 2026-06-01 09:34:10
 * @FilePath: /ROS2_SDK/ros2_ws/src/io_gripper_ros/src/io_gripper_node.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <memory>

#include "gripper_port_resolver.hpp"
#include "io_gripper.hpp"
#include "io_gripper_interfaces/msg/gripper_command.hpp"
#include "io_gripper_interfaces/msg/status.hpp"
#include "io_gripper_interfaces/srv/command_position.hpp"
#include "io_gripper_interfaces/srv/command_velocity.hpp"
#include "io_gripper_interfaces/srv/emergency_stop.hpp"
#include "io_gripper_interfaces/srv/fix_config.hpp"
#include "io_gripper_interfaces/srv/get_status.hpp"
#include "io_gripper_interfaces/srv/pick_object.hpp"
#include "io_gripper_interfaces/srv/scan_ids.hpp"
#include "io_gripper_interfaces/srv/set_effort_limit.hpp"
#include "io_gripper_interfaces/srv/set_soft_limit.hpp"
#include "io_gripper_interfaces/srv/set_torque.hpp"
#include "io_gripper_interfaces/srv/start_polling.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
namespace io::gripper {
// class GripperDriver;

class IoGripperNode : public rclcpp::Node {
 public:
  IoGripperNode();

 private:
  DeviceProfile profile_;

  std::unique_ptr<GripperPortResolver> port_resolver_;

  bool auto_detect_port_;
  std::string camera_serial_;
  std::string port_;
  std::string camera_image_port_;
  std::string config_file_path_;
  std::atomic_bool calibrating_{false};

  bool driver_created_{false};
  bool driver_connected_{false};
  bool driver_initialized_{false};
  std::string last_error_message_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr camera_timer_;
  std::unique_ptr<GripperDriver> driver_;
  std::unique_ptr<GripperCamera> camera_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr connect_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr initialize_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::StartPolling>::SharedPtr
      startpolling_srv_;

  rclcpp::Subscription<io_gripper_interfaces::msg::GripperCommand>::SharedPtr
      target_sub_;
  rclcpp::Publisher<io_gripper_interfaces::msg::Status>::SharedPtr status_pub_;
  rclcpp::Service<io_gripper_interfaces::srv::GetStatus>::SharedPtr
      get_status_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::ScanIds>::SharedPtr scanids_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::SetTorque>::SharedPtr
      settorque_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr disconnect_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr calibrate_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::EmergencyStop>::SharedPtr
      emergencystop_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::SetSoftLimit>::SharedPtr
      set_soft_limit_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::SetEffortLimit>::SharedPtr
      set_effort_limit_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::PickObject>::SharedPtr
      pick_object_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::CommandPosition>::SharedPtr
      command_position_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::CommandVelocity>::SharedPtr
      command_velocity_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr clear_fault_srv_;
  rclcpp::Service<io_gripper_interfaces::srv::FixConfig>::SharedPtr
      fix_config_srv_;

  rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr
      camera_image_pub_;

    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_camera_srv_;



 private:
  bool isDriverReady() const;
  bool createDriver();
  bool connectDriver();
  bool initializeGripper();
  std::string resolvePort();
  std::string resolveCameraImagePort();
  void startpollingCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::StartPolling::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::StartPolling::Response>
          reponse);
  void targetCallback(
      const io_gripper_interfaces::msg::GripperCommand::SharedPtr msg);
  void publishState();

  void connectCallback(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  void initializeCallback(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void scanidsCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::ScanIds::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::ScanIds::Response> response);

  void settorqueCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::SetTorque::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::SetTorque::Response>
          response);
  void disconnectCallback(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  void calibrateCallback(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  void getStatusCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::GetStatus::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::GetStatus::Response>
          response);

  void emergencystopCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::EmergencyStop::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::EmergencyStop::Response>
          respnonse);

  void setSoftLimitCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::SetSoftLimit::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::SetSoftLimit::Response>
          response);
  void setEffortLimitCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::SetEffortLimit::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::SetEffortLimit::Response>
          response);
  void pickObjectCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::PickObject::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::PickObject::Response>
          response);
  void commandPositionCallback(
      const std::shared_ptr<
          io_gripper_interfaces::srv::CommandPosition::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::CommandPosition::Response>
          response);
  void commandVelocityCallback(
      const std::shared_ptr<
          io_gripper_interfaces::srv::CommandVelocity::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::CommandVelocity::Response>
          response);
  void clearFaultCallback(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
      std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void fixConfigCallback(
      const std::shared_ptr<io_gripper_interfaces::srv::FixConfig::Request>
          request,
      std::shared_ptr<io_gripper_interfaces::srv::FixConfig::Response>
          response);

    void publishCameraImage();
    void stopCameraCallback(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response);
};
}  // namespace io::gripper