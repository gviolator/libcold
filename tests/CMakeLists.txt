cmake_minimum_required(VERSION 3.15)

# add_subdirectory(testcommonlib)
add_subdirectory(tests-runtime)

set_property(
	TARGET
		test.runtime
	PROPERTY
		FOLDER "Tests")
