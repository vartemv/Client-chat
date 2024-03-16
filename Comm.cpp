//
// Created by artem on 3/7/24.
//
#include "Comm.h"

const char *HOST = "127.0.0.1";
//const char *HOST = "anton5.fit.vutbr.cz";
uint32_t port_number = 4567;
uint16_t timeout = 250;
uint8_t retransmits = 3;

sockaddr_in server_connection() {

    struct hostent *server = gethostbyname(HOST);
    if (server == nullptr) {
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

int create_socket() {

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_socket <= 0) {
        perror("ERROR: socket");
        return {};
    }
    return client_socket;
}

int receive_message(sockaddr_in server_address, int client_socket, uint8_t *buf, size_t len) {
    socklen_t server_address_length = sizeof(server_address);
    int bytes_received = recvfrom(client_socket, buf, len, 0, (struct sockaddr *) &server_address,
                                  &server_address_length);
    if (bytes_received <= 0) {
        perror("ERROR: recvfrom");
        return 0;
    }
    return bytes_received;
}

void listen_on_socket(sockaddr_in server_address, int client_socket) {
    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    uint8_t buf[2048];
    size_t len = sizeof(buf);
    boost::interprocess::managed_shared_memory segment_bool(boost::interprocess::create_only, "60", 1024);
    bool* listen_on_port = segment_bool.construct<bool>("listening")(true);

    while (*listen_on_port) {
        int ret = poll(fds, 1, 300);

        if (ret == -1) {
            printf("Error: poll failed\n");
            break;
        } else if (!ret) {
            continue;
        } else {
            pid_t pid = fork();
            if (pid == 0) {
                int message_length = receive_message(server_address, client_socket, buf, len);

                if (message_length <= 0)
                    std::cout << "Problem with message" << std::endl;
                std::cout<<message_length<<std::endl;
                if (!decipher_the_message(buf, message_length))
                    *listen_on_port = false;
                kill(getpid(), SIGKILL);
            }
        }
    }
    segment_bool.destroy<bool>("listening");
    boost::interprocess::shared_memory_object::remove("60");
}