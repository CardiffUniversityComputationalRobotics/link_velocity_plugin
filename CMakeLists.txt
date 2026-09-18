cmake_minimum_required(VERSION 3.8)
project(link_velocity_plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(gz-cmake3 REQUIRED)
find_package(gz-common5 REQUIRED)
find_package(gz-plugin2 REQUIRED COMPONENTS register)
find_package(gz-sim8 REQUIRED)
find_package(gz-transport13 REQUIRED)
find_package(gz-msgs10 REQUIRED)
find_package(gz-math7 REQUIRED)

set(GZ_SIM_VER ${gz-sim8_VERSION_MAJOR})
set(GZ_COMMON_VER ${gz-common5_VERSION_MAJOR})
set(GZ_TRANSPORT_VER ${gz-transport13_VERSION_MAJOR})
set(GZ_MSGS_VER ${gz-msgs10_VERSION_MAJOR})
set(GZ_MATH_VER ${gz-math7_VERSION_MAJOR})

add_library(LinkVelocityPlugin SHARED
  src/LinkVelocityPlugin.cpp
)

ament_target_dependencies(LinkVelocityPlugin
  gz-common${GZ_COMMON_VER}
  gz-sim${GZ_SIM_VER}
  gz-transport${GZ_TRANSPORT_VER}
  gz-msgs${GZ_MSGS_VER}
  gz-math${GZ_MATH_VER}
)

install(
  TARGETS LinkVelocityPlugin
  DESTINATION lib/${PROJECT_NAME}
)

ament_environment_hooks("${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.dsv.in")
ament_environment_hooks("${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.sh.in")

ament_package()
