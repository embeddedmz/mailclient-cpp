cmake_minimum_required (VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)

# Read version from file VERSION
FILE(READ "VERSION" project_VERSION)
STRING(STRIP "${project_VERSION}" project_VERSION)

SET(LICENSE "MIT")
cmake_policy(SET CMP0022 NEW)

PROJECT(MailClient VERSION ${project_VERSION} LANGUAGES CXX)

if(NOT MSVC)
	add_definitions(-DLINUX)
else()
	add_definitions(-DWINDOWS)
endif()

option(SKIP_TESTS_BUILD "Skip tests build" ON)

include_directories(MAIL)

add_subdirectory(MAIL)

if(NOT SKIP_TESTS_BUILD)
add_subdirectory(TestMAIL)

include(CTest)
enable_testing ()

# test if the test INI file exist, otherwise default it to the one in TestMAIL folder
IF (NOT TEST_INI_FILE)
    SET(TEST_INI_FILE "./TestMAIL/template_test_conf.ini")
    MESSAGE(WARNING "You didn't provide an INI test configuration file.\
    Defaulting TEST_INI_FILE to ./TestMAIL/template_test_conf.ini")
ENDIF()

add_test (NAME MailClientTest COMMAND test_mailclient ${TEST_INI_FILE})
endif(NOT SKIP_TESTS_BUILD)
