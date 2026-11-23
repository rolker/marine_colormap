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

# Checks that the built library uses the same Qt major as ROS's qt_gui_cpp
# (ADR-0003). Run with cmake -P and these variables:
#   OBJDUMP    objdump executable
#   LIBRARY    the built marine_colormap_widgets library
#   REFERENCE  qt_gui_cpp's library, the oracle
# Every libQt<N>* entry LIBRARY needs must have the major of the one
# libQt<N>Core entry REFERENCE needs. qt_gui_cpp's Core is compared because
# its other Qt entries may be dropped by --as-needed linking.

foreach(var OBJDUMP LIBRARY REFERENCE)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "check_same_qt_major: ${var} is not set")
  endif()
endforeach()

# Sets out_var to the libQt* NEEDED entries of file.
function(needed_qt file out_var)
  execute_process(COMMAND "${OBJDUMP}" -p "${file}"
    OUTPUT_VARIABLE dump RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "objdump -p ${file} failed: ${rc}")
  endif()
  string(REGEX MATCHALL "NEEDED[ \t]+libQt[^ \t\n]*" lines "${dump}")
  set(entries "")
  foreach(line IN LISTS lines)
    string(REGEX REPLACE "^NEEDED[ \t]+" "" entry "${line}")
    list(APPEND entries "${entry}")
  endforeach()
  set(${out_var} "${entries}" PARENT_SCOPE)
endfunction()

needed_qt("${REFERENCE}" ref_entries)
list(FILTER ref_entries INCLUDE REGEX "^libQt[0-9]+Core\\.")
list(LENGTH ref_entries n)
if(NOT n EQUAL 1)
  message(FATAL_ERROR
    "expected one libQt<N>Core in ${REFERENCE}, found: ${ref_entries}")
endif()
string(REGEX REPLACE "^libQt([0-9]+).*" "\\1" ref_major "${ref_entries}")

needed_qt("${LIBRARY}" lib_entries)
if(NOT lib_entries)
  message(FATAL_ERROR "no libQt* entry in ${LIBRARY}")
endif()
foreach(entry IN LISTS lib_entries)
  string(REGEX REPLACE "^libQt([0-9]+).*" "\\1" major "${entry}")
  if(NOT major STREQUAL ref_major)
    message(FATAL_ERROR "Qt major mismatch: ${entry} is ${major}, "
      "qt_gui_cpp's is ${ref_major} (${LIBRARY} needs: ${lib_entries})")
  endif()
endforeach()
message(STATUS "Qt major ${ref_major} as qt_gui_cpp: ${lib_entries}")
