cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Define the library to test
set(LIB_TO_TEST thelz4library)

# Set the directory for test sources
set(TEST_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB TEST_SOURCES ${TEST_SOURCES_DIR}/*.c)

set(CUSTOM_PACKAGES amemorylibrary)

find_package(a-cmake-library REQUIRED)

include(BinaryConfig)
