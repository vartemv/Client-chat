//
// Created by artem on 3/26/24.
//
#include "packets_tcp.h"

void auth_to_server_tcp_logic(std::string &u_n, std::string &disp_name, const char *Token, int client_socket) {
    std::string message = "AUTH " + u_n + " AS " + disp_name + " USING " + std::string(Token) + "\r\n";
    const char *p_message = message.c_str();
    size_t bytes_left = message.size();

    ssize_t bytes_tx = send(client_socket, p_message, bytes_left, 0);
    if (bytes_tx < 0) {
        throw std::runtime_error(std::string("ERROR: send: ") + strerror(errno));
    }

    if (wait_for_reply()) {
        *auth = true;
        *open_state = true;
    }
}

void say_bye_tcp_logic() {}

void join_to_server_tcp_logic(std::string &disp_name, std::string &ch_id) {}

void send_msg_tcp_logic(std::string &d_name, std::string &msg) {}

bool wait_for_reply(){

}