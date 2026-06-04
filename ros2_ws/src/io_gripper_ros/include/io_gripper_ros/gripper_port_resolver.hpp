/*
 * @Author: 培岩 樊 fanpy@io-ai.tech
 * @Date: 2026-05-25 17:57:53
 * @LastEditors: 培岩 樊 fanpy@io-ai.tech
 * @LastEditTime: 2026-05-26 10:22:40
 * @FilePath:
 * /ROS2_SDK/ros2_ws/src/io_gripper_ros/include/io_gripper_ros/gripper_port_resolver.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <string>

#include "io_gripper.hpp"
namespace io::gripper {
class GripperPortResolver {
 public:
  GripperPortResolver();
  ~GripperPortResolver();

  std::string resolveByCameraSerial(const std::string& camera_serial);
  std::string resolveCameraImageByCameraSerial(const std::string& camera_serial);

  void printAllMappings();
  DeviceProfile create_gripper_driver(const std::string& config_file_path);

 private:
  find_port port_finder_;
};
}  // namespace io::gripper