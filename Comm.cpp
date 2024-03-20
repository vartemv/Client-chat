//
// Created by artem on 3/7/24.
//
#include "Comm.h"

////const char *HOST = "127.0.0.1";
//const char *HOST = "anton5.fit.vutbr.cz";
//uint32_t port_number = 4567;
//uint16_t timeout = 250;
//uint8_t retransmits = 3;

sockaddr_in server_connection() {

    struct hostent *server = gethostbyname(HOST_chat);
    if (server == nullptr) {
        fprintf(stderr, "ERROR: no such host %s\n", HOST_chat);
        return {};
    }
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
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

void listen_on_socket(sockaddr_in server_address, int client_socket, SharedVector *myVector) {
    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    uint8_t buf[2048];
    size_t len = sizeof(buf);
    boost::interprocess::shared_memory_object::remove("117");
    boost::interprocess::managed_shared_memory segment_bool(boost::interprocess::create_only, "117", 1024);
    bool *listen_on_port = segment_bool.construct<bool>("listening")(true);

    pid_t main_id = getpid();
    pid_t pid;

    while (*listen_on_port) {
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

                if (!decipher_the_message(buf, message_length, myVector, server_address, client_socket))
                    *listen_on_port = false;

                kill(getpid(), SIGKILL);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    }
    if (getpid() == main_id) {
        segment_bool.destroy<bool>("listening");
        boost::interprocess::shared_memory_object::remove("117");
    }
}