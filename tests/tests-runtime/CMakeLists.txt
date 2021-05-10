cmake_minimum_required(VERSION 3.17)

include(autogroup)

set(TargetName test.runtime)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file (GLOB_RECURSE Sources "*.cpp" "*.h")
auto_group_files("${Sources}" "test_runtime")

add_executable(${TargetName} ${Sources})

# target_link_libraries(${TargetName}
# 	PRIVATE
# 		testcommonlib
# 		remoting
# )

target_precompile_headers(${TargetName} PRIVATE pch.h)
