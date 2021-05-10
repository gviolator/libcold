set_property(GLOBAL PROPERTY USE_FOLDERS ON)

macro (auto_group_files files rootGroupName)

	set (ShortestPathLength 10000)

	foreach (f ${files})

		get_filename_component(DirName ${f} DIRECTORY)

		if (DirName STREQUAL "..")
			continue()
		endif()

		string(LENGTH ${DirName} CurrentLength)

		if (CurrentLength LESS ShortestPathLength)
			set (ShortestPathLength ${CurrentLength})
		endif()

	endforeach()

	foreach (f ${files})

		get_filename_component(GroupName ${f} DIRECTORY)

		string(SUBSTRING "${GroupName}" ${ShortestPathLength} -1 GroupName)

		string(REPLACE "/" "\\" GroupName "${GroupName}")
		
		source_group("${rootGroupName}${GroupName}" FILES ${f} )

	endforeach()

endmacro()


macro (auto_group_files_ files root rootGroupName)

	foreach (originalPath ${files})

		string(REPLACE "${root}" "" cuttedPath "${originalPath}")

		get_filename_component(GroupName ${cuttedPath} DIRECTORY)
		
		string(REPLACE "/" "\\" GroupName "${GroupName}")

		#message ("${originalPath} -> [${GroupName}]")

		source_group("${rootGroupName}${GroupName}" FILES ${originalPath} )

	endforeach()

endmacro()