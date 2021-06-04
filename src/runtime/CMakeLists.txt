cmake_minimum_required(VERSION 3.17)

include(autogroup)
# include(export_includes)

file(GLOB_RECURSE Sources "*.cpp" "*.h")
file(GLOB_RECURSE PlatformSources "platform/${CMAKE_SYSTEM_NAME}/*.cpp" "platform/${CMAKE_SYSTEM_NAME}/*.h")

list(FILTER Sources EXCLUDE REGEX ".*/platform/.*")
list(APPEND Sources ${PlatformSources})

auto_group_files("${Sources}" "runtime")

set(TargetName coldruntime)
add_library(${TargetName} SHARED ${Sources})





# "${CMAKE_SOURCE_DIR}/extras/Visualizers/libcold.natvis"

find_package(LibUv REQUIRED)
# find_package(ICU REQUIRED COMPONENTS uc)
find_package(RapidJSON REQUIRED)
find_package(CryptoPP REQUIRED)


target_compile_definitions(${TargetName}
	PUBLIC
		USING_UV_SHARED=1
	PRIVATE
		RUNTIME_BUILD=1
		NETWORK_BUILD=1
		${TargetName}_EXPORTS=1
	)

target_include_directories(${TargetName}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/network
)


target_include_directories(${TargetName}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

# export_target_default_includes(${TargetName})

target_link_libraries(${TargetName}
	PUBLIC
		LibUv::uv
		rapidjson
		CryptoPP::CryptoPP
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	target_link_libraries(${TargetName}
		PRIVATE
			Rpcrt4
			Ws2_32
			Iphlpapi
	)
endif()

target_precompile_headers(${TargetName} PRIVATE pch.h)


install(TARGETS ${TargetName}
	EXPORT ${ATDRUNTIME_EXPORT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

# ICU::ICU