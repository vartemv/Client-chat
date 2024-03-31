//
// Created by artem on 3/26/24.
//
#include "packets_tcp.h"

void auth_to_server_tcp_logic(std::string &u_n, std::string &disp_name, std::string &Token, int client_socket) {
    std::string message = "AUTH " + u_n + " AS " + disp_name + " USING " + Token + "\r\n";
    send_message_tcp(client_socket, message);
}

void say_bye_tcp_logic(int client_socket) {
    std::string message = "BYE\r\n";
    send_message_tcp(client_socket, message);
}

void join_to_server_tcp_logic(std::string &disp_name, std::string &ch_id, int client_socket) {
    std::string message = "JOIN " + ch_id + " AS " + disp_name + "\r\n";
    send_message_tcp(client_socket, message);
}

void send_msg_tcp_logic(std::string &d_name, std::string &msg, int client_socket, bool error_tcp) {
    std::string message;
    error_tcp ? message = "ERR FROM " + d_name + " IS " + msg + "\r\n" : message = "MSG FROM " + d_name + " IS " + msg +
                                                                                   "\r\n";
    send_message_tcp(client_socket, message);
}

void send_message_tcp(int client_socket, std::string &message) {
    const char *p_message = message.c_str();
    size_t bytes_left = message.size();

    ssize_t bytes_tx = send(client_socket, p_message, bytes_left, 0);
    if (bytes_tx < 0) {
        throw std::runtime_error(std::string("ERROR: send: ") + strerror(errno));
    }

}
