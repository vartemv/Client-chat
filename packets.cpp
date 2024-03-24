//
// Created by artem on 3/8/24.
//
#include <iostream>
#include "packets.h"

const char *TOKEN_IPK = "cc20830f-8124-49d9-b3d4-2d63dfe15bfb";

void increment_counter() {
    sem_wait(counter_stop);
    *count += 1;
    sem_post(counter_stop);
}

/**
 * @brief Checks if a message is confirmed by the server by waiting for a defined period of time and checking if the
 *        message counter in the shared vector is confirmed.
 *
 * @param counter The counter value of the message to be checked.
 * @param client_socket The client socket.
 * @param buf_out The buffer containing the message to be sent.
 * @param len The length of the message in bytes.
 * @param server_address The server address.
 * @param myVector The shared vector containing the message counters.
 *
 * @return True if the message is confirmed, false otherwise.
 */
bool waiting_for_confirm(int counter, int client_socket, uint8_t *buf_out, int len,
                         sockaddr_in *server_address, SharedVector *myVector) {
    long bytes_tx;
    for (int i = 0; i < retransmissions; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_chat));
        sem_wait(sent_messages);
        auto it = std::find(myVector->begin(), myVector->end(), counter);
        sem_post(sent_messages);
        if (it != myVector->end()) {
            if (std::next(it) != myVector->end() && *std::next(it) != 1) {
                std::cout << "Not confirmed, sending again" << std::endl;
                bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address,
                                  sizeof(*server_address));
                if (bytes_tx < 0) perror("ERROR: sendto");
            } else if (*std::next(it) == 1) {
                std::cout << "Message confirmed" << std::endl;
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

/**
 * @brief Add a counter value to a shared vector of messages and fix strange behavior at the beginning of communication.
 *
 * This function adds a counter value to a shared vector of messages
 * The function uses a semaphore to ensure synchronized access to the shared vector.
 *
 * @param myVector A pointer to the shared vector where the counter value will be added.
 * @param counter The counter value to be added to the shared vector.
 */
void add_to_sent_messages(SharedVector *myVector, int counter) {
    sem_wait(sent_messages);
    myVector->push_back(counter);
    myVector->push_back(0);
    sem_post(sent_messages);

    //Bad code to fix strange behaviour of vector at the beginning of comunication
    auto it1 = std::find(myVector->begin(), myVector->end(), 65535);
    auto it2 = std::find(myVector->begin(), myVector->end(), 65266);
    if (it1 != myVector->end()) {
        *it1 = 0;
    }
    if (it2 != myVector->end()) {
        *it2 = 0;
    }
}

/**
 * @brief Authenticate with the server
 *
 * This function sends an authentication packet to the server to authenticate the client.
 * It constructs an authentication packet with the provided username, displayname, and IPK token.
 * The constructed packet is sent to the server using the client_socket and server_address.
 * If the authentication is successful, the auth flag is set to true and the open_state flag is set to true.
 *
 * @param server_address A pointer to the server address structure
 * @param client_socket The client socket descriptor
 * @param u_n The username
 * @param disp_name The display name
 * @param myVector A pointer to the shared vector of messages
 */
void auth_to_server(sockaddr_in *server_address, int client_socket, std::string &u_n, std::string &disp_name,
                    SharedVector *myVector) {
    uint8_t buf_out[256];

    int local_count = *count;
    AuthPacket authPacket(0x02, *count, u_n, disp_name, TOKEN_IPK);
    increment_counter();

    int len = authPacket.construct_message(buf_out);

    int address_size = sizeof(*server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address,
                           address_size);
    if (bytes_tx < 0) {
        perror("ERROR: sendto");
        std::cout << "auth problem" << std::endl;
    }

    add_to_sent_messages(myVector, local_count);

    if (!waiting_for_confirm(local_count, client_socket, buf_out, len, server_address, myVector))
        std::cout << "Couldn't auth to server, try again" << std::endl;
    else{
        *auth = true;
        *open_state = true;
    }
}

/**
 * @brief Sends a "BYE" message to a server
 *
 * This function sends a "BYE" message to a server in the form of a UDP packet.
 * It constructs the message using the provided server address and client socket.
 * It also updates the shared vector of sent messages and checks for confirmation of the message.
 * If the message is confirmed, the function sets the listen_on_port flag to false.
 *
 * @param server_address Pointer to the server address structure
 * @param client_socket The client socket to use for sending the message
 * @param myVector Pointer to the shared vector of sent messages
 */
bool say_bye(sockaddr_in *server_address, int client_socket, SharedVector *myVector) {

    uint8_t buf[3];

    int local_counter = *count;
    Packets bye(0xFF, *count);
    increment_counter();

    int len = bye.construct_message(buf);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) server_address, sizeof(*server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");

    add_to_sent_messages(myVector, local_counter);

    if (!waiting_for_confirm(local_counter, client_socket, buf, len, server_address, myVector)) {
        std::cout << "Couldn't terminate the connection to server, try again" << std::endl;
        return false;
    } else {
        *listen_on_port = false;
        return true;
    }
}


/**
 * @brief Joins the server by sending a JoinPacket to the specified server address.
 *
 * This function constructs a JoinPacket with the specified channel ID and display name, and sends it to the server
 * at the specified server address using the client socket. It also adds the counter value of the JoinPacket to the
 * shared vector of sent messages and waits for confirmation from the server.
 *
 * @param server_address A pointer to the server address.
 * @param client_socket The client socket.
 * @param ch_id The channel ID.
 * @param d_name The display name.
 * @param myVector A pointer to the shared vector of sent messages.
 */
void join_to_server(sockaddr_in *server_address, int client_socket, std::string &ch_id, std::string &d_name,
                    SharedVector *myVector) {
    uint8_t buf[256];

    int local_count = *count;
    JoinPacket join(0x03, *count, ch_id, d_name);
    increment_counter();

    int len = join.construct_message(buf);

    add_to_sent_messages(myVector, local_count);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) server_address, sizeof(*server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");

    if(!waiting_for_confirm(local_count, client_socket, buf, len, server_address, myVector))
        std::cout<<"Couldn't join the channel specified"<<std::endl;

}

void send_msg(sockaddr_in *server_address, int client_socket, std::string &disp_name, std::string &msg, bool error,
              SharedVector *myVector) {
    uint8_t buf_out[1250];

    int local_count = *count;

    MsgPacket message(error ? 0xFE : 0x04, *count, msg, disp_name);
    increment_counter();

    int len = message.construct_message(buf_out);

    socklen_t address_size = sizeof(*server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address, address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    add_to_sent_messages(myVector, local_count);

    if(!waiting_for_confirm(local_count, client_socket, buf_out, len, server_address, myVector))
        std::cout<<"Message haven't been sent"<<std::endl;
}

/**
 * Sends a confirmation packet to the server.
 *
 * @param server_address Pointer to the server address structure.
 * @param client_socket File descriptor of the client socket.
 * @param ref_id Reference ID for the confirmation packet.
 */
void send_confirm(sockaddr_in *server_address, int client_socket, int ref_id) {
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, *count, ref_id);

    int len = confirm.construct_message(buf_out);

    socklen_t address_size = sizeof(*server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address, address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");
}

int read_packet_id(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
}

/**
 * @brief Reads message bytes from a buffer and prints the message.
 *
 * This function reads a sequence of bytes from a buffer and converts them into a string.
 * It then prints the string to the standard output.
 * The message is assumed to be terminated by a byte value of 0x00.
 *
 * @param buf The buffer containing the message bytes.
 * @param message_length The length of the message in bytes.
 */
void read_msg_bytes(uint8_t *buf, int message_length) {
    std::string out_str;
    size_t i = 3;
    for (; i < message_length; ++i) {
        if (buf[i] == 0x00)
            break;
    }
    for (; i < message_length; ++i) {
        out_str += static_cast<char>(buf[i]);
    }
    std::cout << out_str << std::endl;
}

//void identify_reply_type(uint8_t *buf) {
//    int result = buf[3];
//    if (!*open_state) {
//        (!result) ? *auth = false : *auth = true;
//        (!result) ? *open_state = true : *open_state = false;
//    }
//}

/******************************************************************************
 * Function: confirm_id_from_vector
 * --------------------
 * This function confirms the ID from the buffer and updates the corresponding
 * value in the shared vector.
 *
 * buf: A pointer to the buffer containing the data.
 * myVector: A pointer to the shared vector.
 *
 * returns: void
 *****************************************************************************/
void confirm_id_from_vector(uint8_t *buf, SharedVector *myVector) {
    int result = buf[1] << 8 | buf[2];

    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    if (it != myVector->end()) {
        *std::next(it) = 1;
    }
    sem_post(sent_messages);
}

/**
 * @brief Deletes an ID from a vector and sends a confirmation packet.
 *
 * This function takes a buffer, a vector, a server address, and a client socket as input parameters.
 * It extracts an ID from the buffer, finds the ID in the vector, erases it, and sends a confirmation packet.
 * The ID is extracted from the buffer using the read_packet_id function.
 * The confirmation packet is sent using the send_confirm function.
 * This function uses a semaphore to synchronize access to the vector.
 *
 * @param buf The buffer containing the ID.
 * @param myVector The vector from which to delete the ID.
 * @param server_address The server address to send the confirmation packet to.
 * @param client_socket The client socket used to send the confirmation packet.
 */
void delete_id_from_vector(uint8_t *buf, SharedVector *myVector, sockaddr_in *server_address, int client_socket) {

    int result = buf[4] << 8 | buf[5];
    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    if (it != myVector->end()) {
        it = myVector->erase(it);  // erase current item and increment iterator
        myVector->erase(it);
        send_confirm(server_address, client_socket, read_packet_id(buf));
    }
    sem_post(sent_messages);
    std::cout << "\n";
}

/*******************************************************************************
 * Function: decipher_the_message
 * ----------------------------
 * This function deciphers the message based on the first byte of the buffer.
 * It performs different actions based on the value of the first byte.
 * The actions include confirming an ID from a shared vector, deleting an ID from the shared vector
 * and sending a confirmation packet, reading and printing message bytes, handling error and bye messages.
 * The function returns a boolean value indicating whether to continue processing or stop.
 *
 * buf             : A pointer to the buffer containing the message.
 * message_length  : The length of the message in bytes.
 * myVector        : A pointer to the shared vector.
 * server_address  : A pointer to the server address structure.
 * client_socket   : The client socket file descriptor.
 *
 * returns         : True if continue processing, false if stop.
 ******************************************************************************/
bool decipher_the_message(uint8_t *buf, int message_length, SharedVector *myVector, sockaddr_in *server_address,
                          int client_socket) {
    switch (buf[0]) {
        case 0x00://Confirm
            confirm_id_from_vector(buf, myVector);
            break;
        case 0x01://REPLY
            delete_id_from_vector(buf, myVector, server_address, client_socket);
            break;
        case 0x04://MSG
            read_msg_bytes(buf, message_length);
            send_confirm(server_address, client_socket, read_packet_id(buf));//read_packet_id(buf);
            break;
        case 0xFE://ERR
            std::cout << "Err" << std::endl;
            break;
        case 0xFF://BYE
            std::cout << "Bye" << std::endl;
            return false;
        default:
            std::cout << "Def" << std::endl;
            break;
    }
    return true;
}