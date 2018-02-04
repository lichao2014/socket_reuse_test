cmake_minimum_required(VERSION 2.8)

project(socket_reuse_test)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
	link_libraries(Ws2_32)
else()
	link_libraries(pthread)
endif()

add_library(comm socket.h flags.h flags.cc)

link_libraries(comm)
add_executable(tcp_server tcp_server.cc)
add_executable(tcp_client tcp_client.cc)
add_executable(udp_server udp_server.cc)
add_executable(udp_client udp_client.cc)
