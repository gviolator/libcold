cmake_minimum_required(VERSION 3.16)

project(libcold
	LANGUAGES CXX
	VERSION 0.0.1
)

if (NOT CMAKE_BUILD_TYPE)
	message("CMAKE_BUILD_TYPE set: Debug")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#if (PKG_EDITABLE_CONFIGURATION)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target/")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target/")
	message ("Target build directory:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
#endif()


set(LIBCOLD_PACKAGE_NAME ${PROJECT_NAME})
set(LIBCOLD_EXPORT_NAME ${LIBCOLD_PACKAGE_NAME}-targets)
set(LIBCOLD_CONFIG_FILENAME ${LIBCOLD_PACKAGE_NAME}-config.cmake)
set(LIBCOLD_CONFIGVERSION_FILENAME ${LIBCOLD_PACKAGE_NAME}-config-version.cmake)
set(LIBCOLD_CMAKE_DIR cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/_dist/x86_64-${CMAKE_BUILD_TYPE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")





# configure_file(
# 	${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIG_FILENAME}.in
# 	${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIG_FILENAME}
# 	@ONLY
# )

# include(CMakePackageConfigHelpers)

# message("VER FILE:" ${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIGVERSION_FILENAME})

# write_basic_package_version_file(${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIGVERSION_FILENAME}
# 	COMPATIBILITY SameMajorVersion
# )

# install(FILES
# 	${PROJECT_BINARY_DIR}/${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIG_FILENAME}
# 	${PROJECT_BINARY_DIR}/${LIBCOLD_CMAKE_DIR}/${LIBCOLD_CONFIGVERSION_FILENAME}
# 	DESTINATION ${LIBCOLD_CMAKE_DIR}
# )

# install(EXPORT ${LIBCOLD_EXPORT_NAME}
# 	DESTINATION ${LIBCOLD_CMAKE_DIR}
# )

# export(EXPORT ${LIBCOLD_EXPORT_NAME} FILE ${LIBCOLD_CMAKE_DIR}/${LIBCOLD_EXPORT_NAME}.cmake)

# include(${CMAKE_BINARY_DIR}/conan_paths.cmake)


message ("** Configure [${CMAKE_SYSTEM_NAME}] with [${CMAKE_CXX_COMPILER_ID}]: ${CMAKE_BUILD_TYPE}")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") #configure clang -------------------------

		message("!! configure using clang")
		set(LIBCXXROOT )
		file(TO_CMAKE_PATH $ENV{LIBCXX}x86_64-debug LIBCXXROOT)
		message("LIBCXX root:" ${LIBCXXROOT})

	#	set(CL_VERSION 19.28.29334)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -Wclang-cl-pch -Xclang -fdiagnostics-absolute-paths -Xclang -std=c++2a -Xclang -stdlib=libc++  -Xclang -nostdinc++ -I${LIBCXXROOT}/include/c++/v1 -D_CRT_SECURE_NO_WARNINGS -D_CRT_STDIO_ISO_WIDE_SPECIFIERS -D_DEBUG -D_LIBCPP_HAS_NO_RANGES")
	#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-compatibility-version=${CL_VERSION} -Wclang-cl-pch -Xclang -fdiagnostics-absolute-paths -std=c++2a")

	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") #configure Microsoft VC -------------------------
		message("!! configure using msvc")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /JMC /FC /Zc:__cplusplus /std:c++latest /permissive-")
	#	add_compile_definitions(_HAS_DEPRECATED_RESULT_OF)
	endif()

elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# Linux/GCC:
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fdiagnostics-show-option -Wno-attributes -Wno-unused-result" )

	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	# Linux/CLANG:
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fdiagnostics-absolute-paths" )

	endif()

endif()


if ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
	set(ConfigType "Debug")
else()
	set(ConfigType "Release")
endif()


add_subdirectory(src/runtime)

if (COLD_ENABLE_TESTS)
	add_subdirectory(tests)
endif()
