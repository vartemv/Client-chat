//
// Created by artem on 3/26/24.
//

#ifndef IPK_CPP_PACKETS_TCP_H
#define IPK_CPP_PACKETS_TCP_H

#include "packets.h"
#include <sys/socket.h>

void auth_to_server_tcp_logic(std::string &u_n, std::string &disp_name, std::string &Token, int client_socket);

void say_bye_tcp_logic(int client_socket);

void join_to_server_tcp_logic(std::string &disp_name, std::string &ch_id, int client_socket);

void send_msg_tcp_logic(std::string &d_name, std::string &msg, int client_socket, bool error_tcp);

void send_message_tcp(int client_socket, std::string &message);

#endif //IPK_CPP_PACKETS_TCP_H
