cmake_minimum_required(VERSION 3.28)

project(cloth_simulation C)

set(CMAKE_C_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-g3 -Wall -Wextra -pedantic")
include_directories("/opt/homebrew/Cellar/raylib/5.0/include")

add_compile_options(${TARGET_NAME} -Wall -Wextra -Wpedantic)

add_executable(cloth_simulation main.c)

add_library(raylib SHARED IMPORTED)
set_target_properties(raylib PROPERTIES IMPORTED_LOCATION "/opt/homebrew/Cellar/raylib/5.0/lib/libraylib.4.5.0.dylib")
target_link_libraries(${PROJECT_NAME} raylib)


# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()


