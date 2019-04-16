# Include code coverage
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/contrib/cmake)
INCLUDE(CodeCoverage)

# List of tests and their flags
#LIST(APPEND tests_names "tests_without_flags")
#LIST(APPEND tests_flags " ")

LIST(APPEND tests_names "tload")
LIST(APPEND tests_flags "-Wl,--wrap,open -Wl,--wrap,read -Wl,--wrap,close")

# Declare all tests targets
LIST(LENGTH tests_names count)
MATH(EXPR count "${count} - 1")
FOREACH(i RANGE ${count})
	LIST(GET tests_names ${i} test_name)
	LIST(GET tests_flags ${i} test_flags)
	ADD_EXECUTABLE(${test_name} ${test_name}.c ../load.c)
	TARGET_LINK_LIBRARIES(
		${test_name}
		mockx
		-fprofile-arcs -ftest-coverage
	)
	IF( NOT test_flags STREQUAL " ")
		TARGET_LINK_LIBRARIES(${test_name} ${test_flags})
	ENDIF()
	ADD_TEST(${test_name} ${test_name})
ENDFOREACH()

# Coverage settings
SET(COVERAGE_EXCLUDES "'${CMAKE_SOURCE_DIR}/tests/*'" "'${CMAKE_SOURCE_DIR}/libs/libmock/*'")
SETUP_TARGET_FOR_COVERAGE(coverage ctest covdir)
