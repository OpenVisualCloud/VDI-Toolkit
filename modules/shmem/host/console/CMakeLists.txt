cmake_minimum_required(VERSION 3.0)

project(Connect)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    Connect.cpp
    Connect.h
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)