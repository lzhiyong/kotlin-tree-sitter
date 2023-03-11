#
# Copyright © 2023 Github Lzhiyong
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.14.2)

project(tree-sitter-jni)

set(CMAKE_C_COMPILE clang)
set(CMAKE_CXX_COMPILE clang++)

add_library(${PROJECT_NAME} SHARED bundle.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${PROJECT_SOURCE_DIR}/../../../build/src/tree-sitter/lib/include)

target_link_directories(${PROJECT_NAME} PRIVATE 
    ${PROJECT_SOURCE_DIR}/../../../build/native)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Android")
    target_link_libraries(${PROJECT_NAME} tree-sitter-c tree-sitter log)
else()
    target_link_libraries(${PROJECT_NAME} tree-sitter-c tree-sitter)
endif()

