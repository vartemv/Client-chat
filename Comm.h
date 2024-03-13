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

int create_socket();
sockaddr_in server_connection();

#endif //IPK_CPP_COMM_H


