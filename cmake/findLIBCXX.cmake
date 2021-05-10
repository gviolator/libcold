function(LIBCXX_find__)

	if (LIBCXX_PATH)
		file(TO_CMAKE_PATH ${LIBCXX_PATH} Libcxx_Root)
	else()
		if(NOT "$ENV{LIBCXX}" STREQUAL "")
			file(TO_CMAKE_PATH "$ENV{LIBCXX}" Libcxx_Root)
    #   list(APPEND icu_roots "${NATIVE_PATH}")
    #   set(ICU_ROOT "${NATIVE_PATH}"
    #       CACHE PATH "Location of the ICU installation" FORCE)
    endif()
	endif()

	message("LIBCXX Root:(${Libcxx_Root})")

	if(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
		set (Config__ "release")
	else()
		set (Config__ "debug")
	endif()

	# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	# 	set(Arch__ "x86_64")
	# else()
	# 	set(Arch__ "x86")
	# endif()

	set(Libcxx_Target ${Libcxx_Root}/${Arch}-${Config__})

	find_path(Libcxx_Include
		NAMES "vector"
		PATHS "${Libcxx_Target}/include/c++"
		PATH_SUFFIXES "v1"
		NO_DEFAULT_PATH
	)

	# find_library(Libcxx_LibExperimental
	# 	NAMES "c++experimental"
	# 	PATHS "${Libcxx_Target}/lib"
	# 	NO_DEFAULT_PATH
	# )

	#unset(Libcxx_Include CACHE)
	#unset(Libcxx_Lib CACHE)

	set(LIBCXX_INCLUDE_DIR ${Libcxx_Include} PARENT_SCOPE)
#	set(LIBCXX_LIBRARY ${Libcxx_Lib} PARENT_SCOPE)
	set(LIBCXX_LIBRARY_DIR ${Libcxx_Target}/lib PARENT_SCOPE)

endfunction()

LIBCXX_find__()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBCXX
	FOUND_VAR LIBCXX_FOUND
	REQUIRED_VARS
		LIBCXX_INCLUDE_DIR
		LIBCXX_LIBRARY_DIR
	FAIL_MESSAGE "LibC++ not found"
)
