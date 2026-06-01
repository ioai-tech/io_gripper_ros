/*
 * @Author: 培岩 樊 fanpy@io-ai.tech
 * @Date: 2026-05-26 10:03:42
 * @LastEditors: 培岩 樊 fanpy@io-ai.tech
 * @LastEditTime: 2026-05-26 10:29:49
 * @FilePath: /ROS2_SDK/ros2_ws/src/io_gripper_ros/src/main.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <memory>

#include "io_gripper_ros/io_gripper_node.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<io::gripper::IoGripperNode>();
  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}