# This CMake file i snot part of the better-enums repo.
add_library( better-enums INTERFACE )
add_library( better-enums::better-enums ALIAS better-enums )

target_include_directories(
    better-enums
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> )
