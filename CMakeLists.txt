cmake_minimum_required(VERSION 3.10)
project(Atto)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    message(STATUS "Compiling on Windows")
elseif(UNIX)
    message(STATUS "Compiling on Linux/Unix")
endif()

include_directories(Common)

set(COMMON_SOURCES
    Common/HashTable.cpp
    Common/Socket.cpp
    Common/HashTable.h
    Common/Socket.h
    Common/Message.h
)

add_executable(TCPWorker TCPWorker/main.cpp ${COMMON_SOURCES})
add_executable(UDPWorker UDPWorker/main.cpp ${COMMON_SOURCES})

if(WIN32)
    target_link_libraries(TCPWorker ws2_32)
    target_link_libraries(UDPWorker ws2_32)
elseif(UNIX)
    target_link_libraries(TCPWorker pthread)
    target_link_libraries(UDPWorker pthread)
endif()
