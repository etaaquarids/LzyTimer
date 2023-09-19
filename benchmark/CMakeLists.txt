

set(which_to_execute "LzyTimer")
# 包含子项目。
if(${which_to_execute} STREQUAL "Sleep")
	add_executable (benchmark "Sleep.cpp")
endif()

if(${which_to_execute} STREQUAL "LzyTimer")
	add_executable (LzyTimer "LzyTimer.cpp")
endif()