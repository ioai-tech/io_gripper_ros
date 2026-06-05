cmake_minimum_required(VERSION 3.8)
project(io_gripper_ros)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(io_gripper_interfaces REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(sensor_msgs REQUIRED)
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
  sensor_msgs
  ament_index_cpp
  io_gripper_interfaces
)

# 预编译的 io_gripper sdk 库按目标架构选择：
#   x86_64  -> lib/libio_gripper.so
#   aarch64 -> lib_arm/libio_gripper.so
# 两者 SONAME 均为 libio_gripper.so。这里把选中的库拷到构建目录并归一化命名为
# libio_gripper.so，再随包安装到 lib/，可执行用相对 RPATH ($ORIGIN/..) 找到它，
# 这样链接、运行、打包部署都成立（不再依赖源码目录的绝对路径）。
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
  set(IO_GRIPPER_PREBUILT "${IO_GRIPPER_SDK_DIR}/lib_arm/libio_gripper.so")
else()
  set(IO_GRIPPER_PREBUILT "${IO_GRIPPER_SDK_DIR}/lib/libio_gripper.so")
endif()

if(NOT EXISTS "${IO_GRIPPER_PREBUILT}")
  message(FATAL_ERROR "找不到 io_gripper 预编译库: ${IO_GRIPPER_PREBUILT}")
endif()

set(IO_GRIPPER_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}/io_gripper_lib")
file(MAKE_DIRECTORY "${IO_GRIPPER_LIBDIR}")
file(COPY "${IO_GRIPPER_PREBUILT}" DESTINATION "${IO_GRIPPER_LIBDIR}")
get_filename_component(_io_gripper_name "${IO_GRIPPER_PREBUILT}" NAME)
if(NOT _io_gripper_name STREQUAL "libio_gripper.so")
  file(RENAME "${IO_GRIPPER_LIBDIR}/${_io_gripper_name}"
              "${IO_GRIPPER_LIBDIR}/libio_gripper.so")
endif()

target_link_libraries(io_gripper_node
  yaml-cpp
  ${OpenCV_LIBS} 
  ${IO_GRIPPER_LIBDIR}/libio_gripper.so
)

# 预编译库随包安装到 lib/；io_gripper_node 装在 lib/${PROJECT_NAME}/，相对 ../ 即 lib/
install(FILES "${IO_GRIPPER_LIBDIR}/libio_gripper.so"
  DESTINATION lib
)

set_target_properties(io_gripper_node PROPERTIES
  BUILD_RPATH "${IO_GRIPPER_LIBDIR}"
  INSTALL_RPATH "$ORIGIN/.."
)

install(TARGETS io_gripper_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_package()