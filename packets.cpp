//
// Created by artem on 3/8/24.
//
#include <iostream>
#include <regex>
#include "packets_tcp.h"

//const char *TOKEN_IPK = "cc20830f-8124-49d9-b3d4-2d63dfe15bfb";

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
        //std::cout<<"Waiting "<<std::endl;
        if (it != myVector->end()) {
            if (std::next(it) != myVector->end() && *std::next(it) != 1) {
                //std::cout << "Not confirmed, sending again" << std::endl;
                bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address,
                                  sizeof(*server_address));
                if (bytes_tx < 0) perror("ERROR: sendto");
            } else if (*std::next(it) == 1) {
                //std::cout << "Message confirmed" << std::endl;
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
    if (sem_wait(sent_messages) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
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
* @brief Authorizes the client to the server.
*
* This function authorizes the client to the server by sending an authentication packet.
* If the UDP flag is not set, the authorization is done using TCP.
* If the UDP flag is set, the authorization is done using UDP.
*
* @param server_address The server address.
* @param client_socket The client socket.
* @param u_n The username.
* @param disp_name The display name.
* @param myVector A pointer to the shared vector of messages.
*/
void auth_to_server(sockaddr_in *server_address, int client_socket, std::string &u_n, std::string &disp_name,
                    std::string &TOKEN_IPK,
                    SharedVector *myVector) {
    uint8_t buf_out[256];
    if (!*UDP) {
        auth_to_server_tcp_logic(u_n, disp_name, TOKEN_IPK, client_socket);
    } else {
        int local_count = *count;
        AuthPacket authPacket(0x02, *count, u_n, disp_name, TOKEN_IPK);
        increment_counter();

        int len = authPacket.construct_message(buf_out);

        int address_size = sizeof(*server_address);

        long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address,
                               address_size);
        if (bytes_tx < 0) {
            perror("ERROR: sendto");
            std::cerr << "auth problem" << std::endl;
        }

        add_to_sent_messages(myVector, local_count);
        if (!waiting_for_confirm(local_count, client_socket, buf_out, len, server_address, myVector))
            std::cout << "Couldn't auth to server, try again" << std::endl;
        else {
            *auth = true;
            *open_state = true;
        }
    }
}


/**
 * @brief Sends a goodbye message to the server and terminates the connection.
 *
 * The function sends a goodbye message to the server using the provided server address and client socket.
 * If the UDP flag is set, it constructs a UDP packet and sends it via the client socket.
 * If the UDP flag is not set, it calls the `say_bye_tcp_logic` function to handle the goodbye logic over TCP.
 *
 * @param server_address The address of the server.
 * @param client_socket The client socket.
 * @param myVector A pointer to the shared vector of messages.
 *
 * @return True if the connection is terminated successfully, false otherwise.
 */
bool say_bye(sockaddr_in *server_address, int client_socket, SharedVector *myVector) {

    if (!*UDP) {
        say_bye_tcp_logic(client_socket);
        return true;
    } else {

        uint8_t buf[3];

        int local_counter = *count;
        Packet bye(0xFF, *count);
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
}


/**
 * @brief Joins the client to the server.
 *
 * This function is used to join the client to the server. It sends a JoinPacket to the server with the client's
 * display name and channel ID. If UDP is not enabled, it uses TCP logic to join the server. If UDP is enabled,
 * it constructs a JoinPacket, sends it to the server, and waits for confirmation from the server.
 *
 * @param server_address A pointer to the server address.
 * @param client_socket The client socket.
 * @param ch_id The channel ID.
 * @param d_name The client's display name.
 * @param myVector A pointer to the shared vector containing the message counters.
 */
void join_to_server(sockaddr_in *server_address, int client_socket, std::string &ch_id, std::string &d_name,
                    SharedVector *myVector) {
    if (!*UDP) {
        join_to_server_tcp_logic(d_name, ch_id, client_socket);
    } else {
        uint8_t buf[256];

        int local_count = *count;
        JoinPacket join(0x03, *count, ch_id, d_name);
        increment_counter();

        int len = join.construct_message(buf);

        add_to_sent_messages(myVector, local_count);

        long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) server_address, sizeof(*server_address));
        if (bytes_tx < 0) perror("ERROR: sendto");

        if (!waiting_for_confirm(local_count, client_socket, buf, len, server_address, myVector))
            std::cout << "Couldn't join the channel specified" << std::endl;
    }
}

/**
 * @brief Sends a message to a server.
 *
 * This function sends a message to a server using either TCP or UDP based on the value of the global variable `UDP`.
 *
 * @param server_address A pointer to the server address.
 * @param client_socket The client socket.
 * @param disp_name The display name.
 * @param msg The message to send.
 * @param error Flag indicating if the message is an error message.
 * @param myVector A pointer to the shared vector used to track sent messages.
 */
void send_msg(sockaddr_in *server_address, int client_socket, std::string &disp_name, std::string &msg, bool error_msg,
              SharedVector *myVector) {
    if (!*UDP) {
        send_msg_tcp_logic(disp_name, msg, client_socket, error_msg);
    } else {
        uint8_t buf_out[1250];

        int local_count = *count;

        MsgPacket message(error_msg ? 0xFE : 0x04, *count, msg, disp_name);
        increment_counter();

        int len = message.construct_message(buf_out);

        socklen_t address_size = sizeof(*server_address);

        long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) server_address, address_size);
        if (bytes_tx < 0) perror("ERROR: sendto");

        add_to_sent_messages(myVector, local_count);

        if (!waiting_for_confirm(local_count, client_socket, buf_out, len, server_address, myVector))
            std::cout << "Message haven't been sent" << std::endl;
    }
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
void read_msg_bytes(uint8_t *buf, int message_length, bool error_msg) {
    std::string out_str;
    std::string display_name;
    size_t i = 3;
    for (; i < message_length; ++i) {
        display_name += static_cast<char>(buf[i]);
        if (buf[i] == 0x00)
            break;
    }
    for (; i < message_length; ++i) {
        out_str += static_cast<char>(buf[i]);
    }
    if (error_msg)
        std::cerr << "ERROR FROM " << display_name << ": " << out_str << std::endl;
    else
        std::cout << display_name << ": " << out_str << std::endl;
}


/**
 * @brief Confirm the ID from a vector
 *
 * This function takes a buffer and a shared vector as input. It confirms the ID by
 * extracting the value from the buffer, converting it to host byte order, and then
 * searching for the ID in the shared vector. If the ID is found, the value next to
 * it in the vector is set to 1 as message confirmation.
 *
 * @param buf The buffer containing the data
 * @param myVector A shared vector of type uint16_t
 *
 * @return None
 */
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
}

/**
 * @brief Prints the reply message contents.
 *
 * This function takes a buffer and the length of a message and prints the contents
 * of the message. It determines the message contents by scanning the buffer until
 * it encounters a null byte (0x00). The contents are then converted to a std::string.
 * It also checks the value in the third byte of the buffer and prints either
 * "Success: " or "Failure: " before printing the contents.
 *
 * @param buf Pointer to the buffer containing the message.
 * @param message_length The length of the message.
 */
void print_reply(uint8_t *buf, int message_length) {
    size_t i = 6;
    std::string message_contents;
    for (; i < message_length; ++i) {
        message_contents += static_cast<char>(buf[i]);
        if (buf[i] == 0x00) {
            break;
        }
    }
//    ntohs(buf[3]) ? std::cerr << "Success: " << message_contents << std::endl : std::cerr << "Failure: "
//                                                                                          << message_contents
//                                                                                          << std::endl;
    if (ntohs(buf[3])) {
        std::cerr << "Success: " << message_contents << std::endl;
    } else {
        std::cerr << "Failure: " << message_contents << std::endl;
    }

}

void send_err(SharedVector *myVector, sockaddr_in *server_address,
              int client_socket, std::string &DisplayName, std::string &userInput) {
    send_msg(server_address, client_socket, DisplayName, userInput, true, myVector);
    say_bye(server_address, client_socket, myVector);
}

bool check_validity(int from, int to, int actual_length) {
    if (actual_length >= from && actual_length <= to)
        return true;
    std::cerr << "Incorrect message" << std::endl;
    return false;
}

/**
 * @brief Deciphers a message based on the first byte of the buffer.
 *
 * This function takes a buffer, the length of the message, a shared vector, a server address, and a client socket as input.
 * It deciphers the message by switching on the first byte of the buffer and calling different functions accordingly.
 * The functions called depend on the value of the first byte: 0x00, 0x01, 0x04, 0xFE, or 0xFF.
 * Returns true if the message deciphering is successful and the function should continue, otherwise returns false.
 *
 * @param buf The buffer containing the message.
 * @param message_length The length of the message.
 * @param myVector A shared vector of type uint16_t.
 * @param server_address The server address used for sending a confirmation packet.
 * @param client_socket The client socket used for sending a confirmation packet.
 *
 * @return True if the message deciphering is successful and the function should continue, otherwise returns false.
 */
bool decipher_the_message(uint8_t *buf, int message_length, SharedVector *myVector, sockaddr_in *server_address,
                          int client_socket, std::string &DisplayName) {
    switch (buf[0]) {
        case 0x00://Confirm
            if (!check_validity(3, 3, message_length)) {
                std::string userInput = "Incorrect message";
                send_err(myVector, server_address, client_socket, DisplayName, userInput);
                return false;
            }
            confirm_id_from_vector(buf, myVector);
            break;
        case 0x01://REPLY
            if (!check_validity(7, 1407, message_length)) {
                std::string userInput = "Incorrect message";
                send_err(myVector, server_address, client_socket, DisplayName, userInput);
                return false;
            }
            delete_id_from_vector(buf, myVector, server_address, client_socket);
            print_reply(buf, message_length);
            break;
        case 0x04://MSG
            if (!check_validity(5, 1425, message_length)) {
                std::string userInput = "Incorrect message";
                send_err(myVector, server_address, client_socket, DisplayName, userInput);
                return false;
            }
            read_msg_bytes(buf, message_length, false);
            send_confirm(server_address, client_socket, read_packet_id(buf));//read_packet_id(buf);
            break;
        case 0xFE://ERR
            if (!check_validity(5, 1425, message_length)) {
                std::string userInput = "Incorrect message";
                send_err(myVector, server_address, client_socket, DisplayName, userInput);
                return false;
            }
            //std::cout << "Err" << std::endl;
            read_msg_bytes(buf, message_length, true);
            send_confirm(server_address, client_socket, read_packet_id(buf));
            return false;
        case 0xFF://BYE
            if (!check_validity(3, 3, message_length)) {
                std::string userInput = "Incorrect message";
                send_err(myVector, server_address, client_socket, DisplayName, userInput);
                return false;
            }
            std::cout << "Server terminated connection" << std::endl;
            send_confirm(server_address, client_socket, read_packet_id(buf));
            return false;
        default:
            std::string userInput = "Unknown message, terminating the connection";
            std::cerr << userInput << std::endl;
            send_confirm(server_address, client_socket, read_packet_id(buf));
            send_err(myVector, server_address, client_socket, DisplayName, userInput);
            return false;
    }
    return true;
}

/**
 * @brief Deciphers a message using TCP protocol logic.
 *
 * This function takes a message as input and deciphers it based on the TCP protocol logic.
 * The deciphered message is printed to the standard output stream.
 *
 * @param message The message to be deciphered.
 * @return True if the deciphering was successful, false if the message is "BYE".
 */
bool decipher_message_tcp_logic(std::string &message, int client_socket, std::string &d_name) {
    std::istringstream iss(message);
    std::vector<std::string> result;
    for (std::string s; std::getline(iss, s, ' ');) {
        result.push_back(s);
    }

    if (result[0] == "MSG") {
        std::regex e("^MSG FROM .{1,20} IS .{1,1400}$");
        if (std::regex_match(message, e))
            std::cout << "String s matches pattern e\n";
        else {
            std::string mes = "String s does not match pattern e";
            std::cout << mes << std::endl;
            send_msg_tcp_logic(d_name, mes, client_socket, true);
            say_bye_tcp_logic(client_socket);
            return false;
        }

        std::cout << result[2] << ": ";
        for (auto it = std::next(result.begin(), 4); it != result.end(); ++it) {
            if (std::next(it) == result.end()) {
                std::cout << *it;
            } else {
                std::cout << *it << ' ';
            }
        }
        std::cout << std::endl;
    } else if (result[0] == "REPLY") {
        std::regex e("^REPLY (OK|NOK) IS .{1,1400}$");
        if (std::regex_match(message, e))
            std::cout << "String s matches pattern e\n";
        else {
            std::string mes = "String s does not match pattern e";
            std::cout << mes << std::endl;
            send_msg_tcp_logic(d_name, mes, client_socket, true);
            say_bye_tcp_logic(client_socket);
            return false;
        }
        if (result[1] == "OK") {
            if (!*auth) {
                *auth = true;
                *open_state = true;
            }
            std::cerr << "Success:" << ' ';
        } else {
            std::cerr << "Failure:" << ' ';
        }
        for (auto it = std::next(result.begin(), 3); it != result.end(); ++it) {
            if (std::next(it) == result.end()) {
                std::cerr << *it;
            } else {
                std::cerr << *it << ' ';
            }
        }
        std::cerr << std::endl;
    } else if (result[0] == "BYE") {
        std::cout << "Sending bye" << std::endl;
        say_bye_tcp_logic(client_socket);
        return false;
    } else if (result[0] == "ERR") {

        std::regex e("^ERR FROM .{1,20} IS .{1,1400}$");
        if (std::regex_match(message, e))
            std::cout << "String s matches pattern e\n";
        else {
            std::string mes = "String s does not match pattern e";
            std::cout << mes << std::endl;
            send_msg_tcp_logic(d_name, mes, client_socket, true);
            say_bye_tcp_logic(client_socket);
            return false;
        }

        std::cerr << "ERR FROM " << result[2] << ": ";
        for (auto it = std::next(result.begin(), 4); it != result.end(); ++it) {
            if (std::next(it) == result.end()) {
                std::cerr << *it;
            } else {
                std::cerr << *it << ' ';
            }
        }
        std::cerr << std::endl;
        say_bye_tcp_logic(client_socket);
        return false;
    } else {
        std::string error_message = "ERR: Unknown message type " + result[0];
        std::cerr << error_message << result[0] << std::endl;
        send_msg_tcp_logic(d_name, error_message, client_socket, true);
        say_bye_tcp_logic(client_socket);
        return false;
    }
    return true;
}