//
// Created by artem on 3/7/24.
//

#ifndef IPK_CPP_COMM_H
#define IPK_CPP_COMM_H

#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <csignal>
#include <poll.h>
#include "packets.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>

typedef boost::interprocess::allocator<uint16_t, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocator;
typedef boost::interprocess::vector<uint16_t, ShmemAllocator> SharedVector;
extern SharedVector *myVector;;

int create_socket();

sockaddr_in server_connection();

void listen_on_socket(sockaddr_in server_address, int client_socket);

int receive_message(sockaddr_in server_address, int client_socket, uint8_t *buf, size_t len);
#endif //IPK_CPP_COMM_H


