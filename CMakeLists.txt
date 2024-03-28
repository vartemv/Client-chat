cmake_minimum_required(VERSION 3.27)
project(ipk_cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(ipk_cpp main.cpp
        packets.h
        Comm.cpp
        Comm.h
        packets.cpp
        global_declarations.h
        chat_handler.h
        chat_handler.cpp
        packets_tcp.h
        packets_tcp.cpp
)
