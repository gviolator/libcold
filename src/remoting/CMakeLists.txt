cmake_minimum_required(VERSION 3.15)

include(autogroup)

set(TARGET_NAME remoting)

file (GLOB_RECURSE SOURCES "*.cpp" "*.h")
auto_group_files("${SOURCES}" "remoting")

find_package(LibUv REQUIRED)
# find_package(ICU REQUIRED COMPONENTS uc)
find_package(RapidJSON REQUIRED)
# find_package(cryptopp REQUIRED)

add_library(${TARGET_NAME} STATIC ${SOURCES})

target_compile_definitions(${TARGET_NAME}
	PRIVATE
		CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
)


target_include_directories(${TARGET_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../runtime/include
		${RapidJSON_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
		LibUv::libuv
#		RapidJSON::RapidJSON
)

target_precompile_headers(${TARGET_NAME} PRIVATE pch.h)


# cryptopp::cryptopp
# ICU::ICU
