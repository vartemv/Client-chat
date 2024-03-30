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


int create_socket();

void server_connection(sockaddr_in *server_address);

void listen_on_socket(sockaddr_in *server_address, int client_socket, SharedVector *myVector);

int receive_message(sockaddr_in *server_address, int client_socket, uint8_t *buf, size_t len);

bool connect_tcp(int client_socket, sockaddr_in *server_address);

bool receive_message_tcp(int client_socket, uint8_t *buf, size_t len, std::string &d_name);

#endif //IPK_CPP_COMM_H


