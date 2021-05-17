

get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)


add_library(LibUv::libuv_shared STATIC IMPORTED)
add_library(LibUv::libuv_static STATIC IMPORTED)

set (Arch x86_64)
set (_IMPORT_PREFIX ${_IMPORT_PREFIX}/3rdparty/_dist/${Arch}-${CMAKE_BUILD_TYPE}/libuv)


set_target_properties(LibUv::libuv_shared PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
	INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
	IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv.lib"
)

set_target_properties(LibUv::libuv_static PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
	INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
	INTERFACE_LINK_LIBRARIES "${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv_a"
)


# file(COPY ${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})


