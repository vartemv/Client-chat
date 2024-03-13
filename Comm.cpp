//
// Created by artem on 3/7/24.
//
#include "Comm.h"

//const char* HOST = "127.0.0.1";
const char* HOST = "anton5.fit.vutbr.cz";

uint32_t port_number = 4567;

sockaddr_in server_connection(){

    struct hostent *server = gethostbyname(HOST);
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host %s\n", HOST);
        return {};
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    memcpy(&server_address.sin_addr.s_addr, server->h_addr, server->h_length);
    return server_address;
}

int create_socket(){

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_socket <= 0) {
        perror("ERROR: socket");
        return {};
    }
    return client_socket;
}

int receive_message(sockaddr_in server_address, int client_socket, uint8_t buf){
    socklen_t server_address_length = sizeof(server_address);
    int bytes_received = recvfrom(client_socket, &buf, sizeof(buf), 0, (struct sockaddr *) &server_address,
                                  &server_address_length);
    if (bytes_received < 0) {
        perror("ERROR: recvfrom");
        return {};
    }
    return bytes_received;
}