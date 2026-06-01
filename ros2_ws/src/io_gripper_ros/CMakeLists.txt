cmake_minimum_required(VERSION 3.8)
project(io_gripper_ros)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(io_gripper_interfaces REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(io_gripper_node
src/main.cpp
src/gripper_port_resolver.cpp
src/io_gripper_node.cpp
)

set(IO_GRIPPER_SDK_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../io_gripper_sdk
)

target_include_directories(io_gripper_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${IO_GRIPPER_SDK_DIR}/include
)

ament_target_dependencies(io_gripper_node
  rclcpp
  std_srvs
  ament_index_cpp
  io_gripper_interfaces
)

target_link_libraries(io_gripper_node
  yaml-cpp
  ${IO_GRIPPER_SDK_DIR}/lib/libio_gripper.so
)

set_target_properties(io_gripper_node PROPERTIES
  BUILD_RPATH "${IO_GRIPPER_SDK_DIR}/lib"
  INSTALL_RPATH "${IO_GRIPPER_SDK_DIR}/lib"
)

install(TARGETS io_gripper_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_package()