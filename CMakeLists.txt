cmake_minimum_required(VERSION 3.10)
project(thelz4library VERSION 0.1.1)

# Options
option(DEBUG "Enable debugging" OFF)
option(ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)

set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_DOCDIR share/doc/thelz4library)
set(CMAKE_C_STANDARD 99)

# Set C standard
set(CMAKE_C_STANDARD 99)

# Find the required libraries
find_package(amemorylibrary REQUIRED)
find_package(themacrolibrary REQUIRED)

# Compiler options
if(ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address)
endif()

# Source files
set(SOURCE_FILES src/lz4.c)

# Debug library
add_library(thelz4library_debug STATIC ${SOURCE_FILES})
target_include_directories(thelz4library_debug PUBLIC ${themacrolibrary_INCLUDE_DIRS})
target_include_directories(thelz4library_debug PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(thelz4library_debug PUBLIC -D_AML_DEBUG_)
target_compile_options(thelz4library_debug PRIVATE -g -O0)
set_target_properties(thelz4library_debug PROPERTIES OUTPUT_NAME "thelz4library_debug")
target_compile_options(thelz4library_debug PRIVATE -g -O0)


# Release library
add_library(thelz4library_static STATIC ${SOURCE_FILES})
target_include_directories(thelz4library_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(thelz4library_static PUBLIC ${themacrolibrary_INCLUDE_DIRS})
target_compile_options(thelz4library_static PRIVATE -O3)
set_target_properties(thelz4library_static PROPERTIES OUTPUT_NAME "thelz4library_static")
target_compile_options(thelz4library_static PRIVATE -O3)

# Shared library
add_library(thelz4library SHARED ${SOURCE_FILES})
target_include_directories(thelz4library PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(thelz4library PUBLIC ${themacrolibrary_INCLUDE_DIRS})
target_compile_options(thelz4library PRIVATE -O3)
set_target_properties(thelz4library PROPERTIES OUTPUT_NAME "thelz4library")
target_compile_options(thelz4library PRIVATE -O3)

# Link libraries
target_link_libraries(thelz4library_debug PUBLIC amemorylibrary)
target_link_libraries(thelz4library_static PUBLIC amemorylibrary)
target_link_libraries(thelz4library PUBLIC amemorylibrary)

# Installation of the library
install(TARGETS thelz4library thelz4library_static thelz4library_debug
        EXPORT thelz4libraryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include)

# Export the targets
install(EXPORT thelz4libraryTargets
        FILE thelz4libraryTargets.cmake
        DESTINATION lib/cmake/thelz4library)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "thelz4libraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_file(thelz4libraryConfig.cmake.in thelz4libraryConfig.cmake @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/thelz4libraryConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/thelz4libraryConfigVersion.cmake"
        DESTINATION lib/cmake/thelz4library)

# Installation of other files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/the-lz4-library DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md AUTHORS NEWS.md CHANGELOG.md LICENSE NOTICE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Uninstall command
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
