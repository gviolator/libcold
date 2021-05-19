cmake_minimum_required(VERSION 3.15)

add_subdirectory(testcommonlib)
add_subdirectory(test-runtime)

set_property(
	TARGET
		testcommonlib
		test.runtime
	PROPERTY FOLDER "Tests")
