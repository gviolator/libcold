cmake_minimum_required(VERSION 3.15)

include(autogroup)

set(TargetName testcommonlib)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file (GLOB_RECURSE Sources "*.cpp" "*.h")
auto_group_files("${Sources}" "testcommonlib")

add_library(${TargetName} STATIC ${Sources})

find_package(GTest REQUIRED)

target_include_directories(${TargetName}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

# target_precompile_headers(${TargetName} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")

target_compile_definitions(${TargetName}
	PUBLIC
		GTEST_LANG_CXX11=1
)

target_link_libraries(${TargetName}
	PUBLIC
		coldruntime
		GTest::gtest
)
