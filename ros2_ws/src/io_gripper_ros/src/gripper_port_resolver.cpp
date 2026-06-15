/*
 * @Author: 培岩 樊 fanpy@io-ai.tech
 * @Date: 2026-05-25 18:00:29
 * @LastEditors: 培岩 樊 fanpy@io-ai.tech
 * @LastEditTime: 2026-05-29 16:53:23
 * @FilePath: /ROS2_SDK/ros2_ws/src/io_gripper_ros/src/gripper_port_resolver.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "io_gripper_ros/gripper_port_resolver.hpp"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <stdexcept>
#include <vector>
namespace io::gripper {
GripperPortResolver::GripperPortResolver() {}
GripperPortResolver::~GripperPortResolver() {}

std::string GripperPortResolver::resolveByCameraSerial(
    const std::string& camera_serial) {
  if (camera_serial.empty()) {
    throw std::runtime_error("camera_serial is empty");
  }

  std::string gripper_path =
      port_finder_.resolve_gripper_by_camera_serial(camera_serial);

  if (gripper_path.empty()) {
    throw std::runtime_error(
        "resolve_gripper_by_camera_serial returned empty path");
  }

  std::string tty_port = port_finder_.find_by_path_from_tty(gripper_path);

  if (tty_port.empty()) {
    throw std::runtime_error("find_by_path_from_tty returned empty port");
  }

  return tty_port;
}


void GripperPortResolver::printAllMappings() {
  std::vector<CameraInfo> cameras = port_finder_.get_usb_cameras_info();

  if (cameras.empty()) {
    std::cout << "No USB camera found." << std::endl;
    return;
  }

  for (size_t i = 0; i < cameras.size(); ++i) {
    std::cout << "[" << i << "] " << std::endl;
    const auto& cam = cameras[i];
    if(cam.serial == "01.00.00"){
      std::string camera_vedio_path =
        port_finder_.find_video_by_camera_serial(cam.serial);
        std::string camera_id_path = port_finder_.get_video_id_path(camera_vedio_path);
        std::string res_camera_video_path = port_finder_.find_video_by_id_path(camera_id_path);
        std::cout << "camera_serial=" << cam.serial
              << ", res_camera_video_path=" << res_camera_video_path
              << ", camera_id_path=" << camera_id_path
              << std::endl;
        continue;
    }
    std::string gripper_path =
        port_finder_.resolve_gripper_by_camera_serial(cam.serial);

    std::string tty_port = port_finder_.find_by_path_from_tty(gripper_path);

    std::cout<< "camera_serial=" << cam.serial
              << ", gripper_path=" << gripper_path << ", tty_port=" << tty_port
              << std::endl;
  }
}


std::string GripperPortResolver::resolveCameraImageByCameraSerial(
    const std::string& camera_serial) {
    if (camera_serial.empty()) {
    throw std::runtime_error("camera_serial is empty");
  }
    std::string gripper_path =
        port_finder_.find_video_by_camera_serial(camera_serial);

    if (gripper_path.empty()) {
      throw std::runtime_error(
          "find_video_by_camera_serial returned empty path");
    }

    std::string dev_vedio_port = port_finder_.get_video_id_path(gripper_path);

    if (dev_vedio_port.empty()) {
      throw std::runtime_error("get_video_id_path returned empty port");
    }

    std::string res_camera_image_port = port_finder_.find_video_by_id_path(dev_vedio_port);

    if (res_camera_image_port.empty()) {
      throw std::runtime_error("find_video_by_id_path returned empty port");
    }

    return res_camera_image_port;
}

DeviceProfile GripperPortResolver::create_gripper_driver(
    const std::string& config_file_path) {
  DeviceProfile profile;
  YAML::Node config = YAML::LoadFile(config_file_path);
  auto DeviceProfile_Node = config["DeviceProfile"];
  // 基础信息
  profile.model_name =
      DeviceProfile_Node["device_info"]["model_name"].as<std::string>();
  profile.servo_id =
      static_cast<int>(DeviceProfile_Node["device_info"]["servo_id"].as<int>());
  profile.recommended_baudrate =
      DeviceProfile_Node["device_info"]["recommended_baudrate"].as<int>();

  // 安全限制
  profile.min_voltage_V =
      DeviceProfile_Node["safety_limits"]["min_voltage_v"].as<float>();
  profile.max_voltage_V =
      DeviceProfile_Node["safety_limits"]["max_voltage_v"].as<float>();
  profile.max_temperature_C =
      DeviceProfile_Node["safety_limits"]["max_temperature_c"].as<float>();
  profile.start_power =
      DeviceProfile_Node["safety_limits"]["start_power"].as<int>(8);
  profile.release_torque_on_disconnect =
      DeviceProfile_Node["safety_limits"]["release_torque_on_disconnect"]
          .as<bool>(true);
  profile.max_servo_velocity =
      DeviceProfile_Node["safety_limits"]["max_servo_velocity"].as<int>(2000);

  // 标定参数
  profile.calib_max_position_raw =
      DeviceProfile_Node["calibration"]["calib_max_position_raw"]
          .as<uint16_t>();
  profile.calib_min_position_raw =
      DeviceProfile_Node["calibration"]["calib_min_position_raw"]
          .as<uint16_t>();
  profile.calib_max_width_mm =
      DeviceProfile_Node["calibration"]["calib_max_width_mm"].as<float>();
  profile.calib_min_width_mm =
      DeviceProfile_Node["calibration"]["calib_min_width_mm"].as<float>();

  // 相机参数
  profile.width =
      DeviceProfile_Node["camera"]["width"].as<int>();
  profile.height =
      DeviceProfile_Node["camera"]["height"].as<int>();
  profile.fps =
      DeviceProfile_Node["camera"]["fps"].as<float>();
  profile.jpeg_quality =
      DeviceProfile_Node["camera"]["jpeg_quality"].as<int>();

  return profile;
}

}  // namespace io::gripper