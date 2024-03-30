//
// Created by artem on 3/7/24.
//
#include "Comm.h"

/**
 * @brief Connects to a server using the specified server address.
 *
 * This function uses the `gethostbyname` function to retrieve the IP address of the server
 * specified by the `HOST_chat` constant. It then initializes the server address structure
 * and copies the retrieved IP address into it, along with the specified port number. The
 * function assumes that the `server_address` pointer is valid and points to a valid
 * `sockaddr_in` structure.
 *
 * If the `gethostbyname` function fails to retrieve the IP address, an error message is printed
 * to `stderr` and the function returns.
 *
 * @param[in] server_address A pointer to the `sockaddr_in` structure that will hold the server address.
 *
 * @return None.
 */
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

/**
 * @brief Creates a socket based on the value of UDP flag.
 *
 * This function creates a socket based on the value of the UDP flag. If the
 * flag is true, a UDP socket is created using the AF_INET and SOCK_DGRAM
 * parameters. If the flag is false, a TCP socket is created using the AF_INET
 * and SOCK_STREAM parameters.
 *
 * @return The created socket file descriptor if successful, otherwise an empty
 * vector is returned.
 */
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

/**
 * Receives a message from a socket.
 *
 * This function receives a message from the specified `client_socket` and stores it in the `buf` buffer.
 * The `len` parameter specifies the maximum number of bytes to be received and stored in the buffer.
 * The `server_address` parameter is a pointer to the sockaddr_in structure that contains the server address
 * from where the message is received. Upon successful reception, the number of bytes received is returned.
 *
 * @param server_address - A pointer to the sockaddr_in structure that contains the server address
 * @param client_socket - The client socket to receive the message from
 * @param buf - Pointer to the buffer to store the received message
 * @param len - The maximum number of bytes to receive and store in `buf`
 * @return The number of bytes received if successful, otherwise 0
 */
int receive_message(sockaddr_in *server_address, int client_socket, uint8_t *buf, size_t len) {
    socklen_t server_address_length = sizeof(*server_address);
    int bytes_received;
    bytes_received = recvfrom(client_socket, buf, len, 0, (struct sockaddr *) server_address,
                              &server_address_length);
    if (bytes_received <= 0) {
        perror("ERROR: recvfrom");
        return 0;
    }
    return bytes_received;
}


/**
 * @brief Receives a message over TCP from a client socket.
 *
 * This function receives a message from a client socket using the recv() system call.
 * It reads the specified number of bytes from the socket and stores them in the buffer.
 * The received message is then converted to a string and passed to the
 * decipher_message_tcp_logic() function for further processing.
 *
 * @param client_socket The client socket to receive the message from.
 * @param buf The buffer to store the received message.
 * @param len The number of bytes to receive.
 * @return True if the message was successfully received and deciphered, false otherwise.
 */
bool receive_message_tcp(int client_socket, uint8_t *buf, size_t len, std::string &d_name) {
    int bytes_received = recv(client_socket, buf, len, 0);
    if (bytes_received < 0) {
        perror("Error");
        return false;
    } else if (bytes_received == 0) {
        std::cout << "Server closed the connection" << std::endl;
        return false;
    }
    std::string out_str;
    for (int i = 0; i < bytes_received - 2; ++i) {
        out_str += static_cast<char>(buf[i]);
    }
    return decipher_message_tcp_logic(out_str, client_socket, d_name);
}

/**
 * Listens for incoming messages on a socket.
 *
 * This function listens for incoming messages on the specified `client_socket` and processes them accordingly.
 * The function uses the `poll` system call to check for any incoming data on the socket. If data is available,
 * it receives the message using the `receive_message` function and then deciphers the message using the
 * `decipher_the_message` function. In case of "BYE", decipher message returns false, thus terminating the connection.
 *
 * @param server_address - A pointer to the sockaddr_in structure that contains the server address
 * @param client_socket - The client socket to listen on
 * @param myVector - A pointer to the shared vector to store the deciphered message (type: `SharedVector`)
 */
bool listen_on_socket(sockaddr_in *server_address, int client_socket, SharedVector *myVector) {
    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    uint8_t buf[4096];
    size_t len = sizeof(buf);

    pid_t main_id = getpid();
    pid_t pid;
    while (*listen_on_port) {
        int ret = poll(fds, 1, 500);

        if (ret == -1) {
            printf("Error: poll failed\n");
            break;
        } else if (ret > 0) {
            if (main_id == getpid())

                pid = fork();

            if (pid == 0) {
                int message_length = receive_message(server_address, client_socket, buf, len);

                if (!decipher_the_message(buf, message_length, myVector, server_address, client_socket))
                    *listen_on_port = false;

                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    if(getpid() == main_id)
        return false;
    return true;
}

bool connect_tcp(int client_socket, sockaddr_in *server_address) {
    if (connect(client_socket, (struct sockaddr *) server_address, sizeof(*server_address)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return false;
    }
    return true;
}