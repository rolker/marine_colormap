# Copyright 2026 Roland Arsenault
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8)
project(marine_colormap)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

# SHARED so the library is position-independent and can link into downstream
# shared libraries / rviz plugins (a default STATIC archive lacks -fPIC and
# fails to link into a .so). Shared is also the ROS 2 convention.
add_library(${PROJECT_NAME} SHARED
  src/palette.cpp
  src/perceptual_palettes.cpp
  src/transfer.cpp
  src/colormap.cpp
  src/lookup.cpp
  src/occupancy.cpp
  src/topobathy_palettes.cpp
  src/shader.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

install(
  DIRECTORY include/
  DESTINATION include)

# Ship the licence and the third-party notices with the installed package. MIT
# requires its permission notice to accompany redistributed copies, and a binary
# install is a redistribution -- leaving these in the source tree only would be
# a compliance gap for anyone consuming the built package.
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
  DESTINATION share/${PROJECT_NAME})
install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)

# Public interface depends on nothing (no Qt / Ogre / ROS) -> no
# ament_export_dependencies. Downstream consumers link the target only.
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_include_directories(include)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  foreach(test_name palette transfer colormap shader lookup occupancy topobathy)
    ament_add_gtest(test_${test_name} test/test_${test_name}.cpp)
    target_link_libraries(test_${test_name} ${PROJECT_NAME})
  endforeach()
endif()

ament_package()
