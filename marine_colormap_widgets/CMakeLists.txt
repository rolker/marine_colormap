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
project(marine_colormap_widgets)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Q_OBJECT (signals/slots) needs moc; AUTOMOC runs it over the sources/headers,
# otherwise the link fails with undefined vtable / signal symbols.
set(CMAKE_AUTOMOC ON)

find_package(ament_cmake REQUIRED)
find_package(marine_colormap REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# SHARED so the widget can link into downstream shared libraries / rqt/rviz
# plugins (a default STATIC archive lacks -fPIC). Shared is the ROS 2 convention.
# The Q_OBJECT header is listed as a source so AUTOMOC scans it: the header lives
# in include/ (not beside the .cpp), so AUTOMOC's default sibling lookup would
# otherwise miss it and emit no moc (undefined vtable / staticMetaObject).
add_library(${PROJECT_NAME} SHARED
  src/colormap_legend_widget.cpp
  include/marine_colormap_widgets/colormap_legend_widget.hpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(${PROJECT_NAME}
  marine_colormap::marine_colormap
  Qt5::Widgets)

install(
  DIRECTORY include/
  DESTINATION include)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)

# Downstream consumers link the exported target; they also need marine_colormap
# (public header re-exports RangeModel/Rgba8) and Qt5 Widgets (public QWidget base).
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_include_directories(include)
ament_export_dependencies(marine_colormap Qt5Widgets)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  # The widget test needs a QApplication; run it headless under the offscreen
  # Qt platform plugin (pulled by the qtbase5-dev rosdep dep).
  ament_add_gtest(test_colormap_legend_widget
    test/test_colormap_legend_widget.cpp
    ENV QT_QPA_PLATFORM=offscreen)
  # Qt5::Widgets is pulled in transitively via ${PROJECT_NAME}'s public link.
  target_link_libraries(test_colormap_legend_widget ${PROJECT_NAME})
endif()

ament_package()
