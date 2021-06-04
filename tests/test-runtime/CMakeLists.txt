cmake_minimum_required(VERSION 3.17)

include(autogroup)

set(TargetName test.runtime)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(GTest REQUIRED)

file (GLOB_RECURSE Sources "*.cpp" "*.h")
file (GLOB_RECURSE RemotingSources "${CMAKE_CURRENT_SOURCE_DIR}/../../src/runtime/remoting/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../../src/runtime/remoting/*.h")


auto_group_files("${Sources}" "test_runtime")

add_executable(${TargetName} ${Sources} ${RemotingSources})

target_link_libraries(${TargetName}
	PRIVATE
		testcommonlib
		GTest::gtest_main
)

target_precompile_headers(${TargetName} PRIVATE pch.h)
