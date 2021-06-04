

get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)


add_library(LibUv::uv STATIC IMPORTED)
add_library(LibUv::uv_static STATIC IMPORTED)

set (Arch x86_64)

# if (CMAKE_GENERATOR MATCHES "Visual Studio")
# 	message ("IM VISUAL")
# else()
# 	message ("NOT VISUAL")
# endif()


set (_IMPORT_PREFIX ${_IMPORT_PREFIX}/3rdparty/_dist/${Arch}-${CMAKE_BUILD_TYPE}/libuv)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")




	set_target_properties(LibUv::uv PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_COMPILE_DEFINITIONS USING_UV_SHARED=1
		IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv.lib"
	)

	set_target_properties(LibUv::uv_static PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_LINK_LIBRARIES "${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv_a.lib"
	)

elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

	set_target_properties(LibUv::uv PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
		INTERFACE_COMPILE_DEFINITIONS USING_UV_SHARED=1
		IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/libuv.so"
	)


endif()


# file(COPY ${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})


