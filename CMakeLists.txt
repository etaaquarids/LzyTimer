# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("LzyTimer")
set(CMAKE_CXX_STANDARD 20)

set(which_to_execute "benchmark")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# 包含子项目。
if(${which_to_execute} STREQUAL "benchmark")
	add_subdirectory ("benchmark")
endif()

if(${which_to_execute} STREQUAL "unitest")
	add_subdirectory ("unitest")
endif()