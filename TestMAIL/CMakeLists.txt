cmake_minimum_required(VERSION 2.6)

project(TestMAILClient)
set(EXECUTABLE_OUTPUT_PATH bin/${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_STANDARD 14) # c++14
add_definitions(-DLINUX)

# Code coverage setup
IF(CMAKE_BUILD_TYPE MATCHES Coverage)
   INCLUDE(CodeCoverage.cmake)
   SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
   SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
ENDIF(CMAKE_BUILD_TYPE MATCHES Coverage)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Locate libcURL
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include_directories(../MAIL)
include_directories(./simpleini)
include_directories(./)

IF(CMAKE_BUILD_TYPE MATCHES Coverage)

file(GLOB_RECURSE mail_source_files ../MAIL/*)

#Output Setup
add_executable(test_mailclient main.cpp test_utils.cpp ${mail_source_files})

#Link setup
target_link_libraries(test_mailclient ${GTEST_LIBRARIES} pthread curl)

SETUP_TARGET_FOR_COVERAGE(
           coverage_mailclient
           test_mailclient
           coverage
           "/home/amzoughi/Test/amine_mail_conf.ini"
           )

ELSE(CMAKE_BUILD_TYPE MATCHES Coverage)

link_directories(../lib/${CMAKE_BUILD_TYPE})

#Output Setup
add_executable(test_mailclient main.cpp test_utils.cpp)

#Link setup
target_link_libraries(test_mailclient mailclient ${GTEST_LIBRARIES} pthread curl)

ENDIF(CMAKE_BUILD_TYPE MATCHES Coverage)
