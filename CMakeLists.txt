cmake_minimum_required(VERSION 3.8)
project(link_velocity_plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)

find_package(gz_common_vendor REQUIRED)
find_package(gz-common REQUIRED)
find_package(gz_math_vendor REQUIRED)
find_package(gz-math REQUIRED)
find_package(gz_msgs_vendor REQUIRED)
find_package(gz-msgs REQUIRED)
find_package(gz_plugin_vendor REQUIRED)
find_package(gz-plugin REQUIRED COMPONENTS register)
find_package(gz_sim_vendor REQUIRED)
find_package(gz-sim REQUIRED)
find_package(gz_transport_vendor REQUIRED)
find_package(gz-transport REQUIRED)

add_library(LinkVelocityPlugin SHARED
  src/LinkVelocityPlugin.cpp
)

target_link_libraries(LinkVelocityPlugin
  gz-common::core
  gz-math::core
  gz-msgs::core
  gz-plugin::register
  gz-sim::core
  gz-transport::core
)

install(
  TARGETS LinkVelocityPlugin
  DESTINATION lib/${PROJECT_NAME}
)

ament_environment_hooks("${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.dsv.in")
ament_environment_hooks("${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.sh.in")

ament_package()
