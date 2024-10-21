cmake_minimum_required(VERSION 3.20)

set(MY_Project_Name Proxy-Server)

project(${MY_Project_Name})

file(GLOB HEADERS
    ${CMAKE_SOURCE_DIR}/include/*.h
)

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
)

find_package(CURL REQUIRED) 

add_executable(Proxy-Server-CMake ${HEADERS} ${SOURCES})

target_link_libraries(Proxy-Server-CMake CURL::libcurl)