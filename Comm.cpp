//
// Created by artem on 3/7/24.
//
#include "Comm.h"

void server_connection(sockaddr_in *server_address) {
    struct hostent *server = gethostbyname(HOST_chat);
    if (server == nullptr) {
        fprintf(stderr, "ERROR: no such host %s\n", HOST_chat);
        return;
    }

    memset(server_address, 0, sizeof(*server_address));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port);
    memcpy(&server_address->sin_addr.s_addr, server->h_addr, server->h_length);
}

int create_socket() {
    int client_socket;
    if (*UDP)
        client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    else
        client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket <= 0) {
        perror("ERROR: socket");
        return {};
    }
    return client_socket;
}

int receive_message(sockaddr_in *server_address, int client_socket, uint8_t *buf, size_t len) {
    socklen_t server_address_length = sizeof(*server_address);
    int bytes_received;
    if (*UDP)
        bytes_received = recvfrom(client_socket, buf, len, 0, (struct sockaddr *) server_address,
                                  &server_address_length);
    else {
        bytes_received = recv(client_socket, buf, len, 0);
    }

    if (bytes_received <= 0) {
        perror("ERROR: recvfrom");
        return 0;
    }
    return bytes_received;
}

void listen_on_socket(sockaddr_in *server_address, int client_socket, SharedVector *myVector) {
    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    uint8_t buf[4096];
    size_t len = sizeof(buf);

    pid_t main_id = getpid();
    pid_t pid;

    while (*listen_on_port) {

        if(!*UDP)
            sem_wait(tcp_listening);

        int ret = poll(fds, 1, 300);

        if (ret == -1) {
            printf("Error: poll failed\n");
            break;
        } else if (ret > 0) {
            if (main_id == getpid())
                pid = fork();
            if (pid == 0) {
                int message_length = receive_message(server_address, client_socket, buf, len);

                if (message_length <= 0)
                    std::cout << "Problem with message" << std::endl;

                if (*UDP) {
                    if (!decipher_the_message(buf, message_length, myVector, server_address, client_socket))
                        *listen_on_port = false;
                }else{
                    //std::cout<<"Here"<<std::endl;
                    if (!decipher_message_tcp_logic(buf, message_length))
                        *listen_on_port = false;
                }

                //kill(getpid(), SIGKILL);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if(!*UDP)
            sem_post(tcp_listening);

    }
}

bool connect_tcp(int client_socket, sockaddr_in *server_address) {
    std::cout << "Connecting" << std::endl;
    if (connect(client_socket, (struct sockaddr *) server_address, sizeof(*server_address)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return false;
    }
    return true;
}