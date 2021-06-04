

# get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
# get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
# get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
# get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
# get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
# get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)


add_library(CryptoPP::CryptoPP STATIC IMPORTED)
set (Arch x86_64)


set (_IMPORT_PREFIX h:/projects/libcold/3rdparty/cryptopp)

set_target_properties(CryptoPP::CryptoPP PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}"
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}"
)


if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

	set_target_properties(CryptoPP::CryptoPP PROPERTIES
		IMPORTED_LOCATION "h:/projects/libcold/3rdparty/_build/cryptopp-x86_64-Debug/cryptopp.lib"
	)

elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")


endif()


# file(COPY ${_IMPORT_PREFIX}/lib/${CMAKE_BUILD_TYPE}/uv.dll DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})


