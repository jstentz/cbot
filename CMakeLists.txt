cmake_minimum_required(VERSION 3.10.0)

project(Cbot)

include_directories(include)

target_include_directories(include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HEADERS "include/*.h")

file(GLOB SOURCES "src/*.cpp")

add_executable(Cbot ${SOURCES} ${HEADERS})

