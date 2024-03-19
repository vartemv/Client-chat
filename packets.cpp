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

bool waiting_for_confirm(int counter, int client_socket, uint8_t *buf_out, int len, int address_size, sockaddr_in server_address){
    long bytes_tx;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        sem_wait(sent_messages);
        auto it = std::find(myVector->begin(), myVector->end(), counter);
        sem_post(sent_messages);
        if (it != myVector->end()) {
            if(std::next(it)!= myVector->end() && *std::next(it) != 1){
                std::cout<<"Not confirmed, sending again"<< std::endl;
                bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address,
                                  address_size);
                if (bytes_tx < 0) perror("ERROR: sendto");
            }else if(*std::next(it) == 1){
                std::cout<<"Message confirmed"<<std::endl;
                return true;
            }
        } else {
            std::cout << "Element " << counter << " not found in vector\n";
            return false;
        }
    }
    return false;
}

void auth_to_server(sockaddr_in server_address, int client_socket, std::string &u_n, std::string &disp_name) {
    uint8_t buf_out[256];
    //uint8_t buf_in[256];

    int local_count = *count;
    AuthPacket auth(0x02, *count, u_n, disp_name, TOKEN_IPK);
//    *count += 1;
    increment_counter();

    int len = auth.construct_message(buf_out);

    printf("INFO: Server socket: %s : %d \n",
           inet_ntoa(server_address.sin_addr),
           ntohs(server_address.sin_port));

    int address_size = sizeof(server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address,
                           address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    waiting_for_confirm(local_count, client_socket, buf_out, len, address_size, server_address);

}

void say_bye(sockaddr_in server_address, int client_socket) {

    uint8_t buf[3];

    Packets bye(0xFF, *count);
    increment_counter();

    int len = bye.construct_message(buf);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");
}

void join_to_server(sockaddr_in server_address, int client_socket, std::string &ch_id, std::string &d_name) {
    uint8_t buf[256];

    JoinPacket join(0x03, *count, ch_id, d_name);
    increment_counter();

    int len = join.construct_message(buf);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");
}

void send_msg(sockaddr_in server_address, int client_socket, std::string &disp_name, std::string &msg, bool error) {
    uint8_t buf_out[1250];

    MsgPacket message(error ? 0xFE : 0x04, *count, msg, disp_name);
    increment_counter();

    int len = message.construct_message(buf_out);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");
}

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

void confirm_id_from_vector(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];

    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    *std::next(it) = 1;
    sem_post(sent_messages);
}

void delete_id_from_vector(uint8_t *buf) {

    int result = buf[4] << 8 | buf[5];
    //std::cout<<"RefId is "<<result<<std::endl;
    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    it = myVector->erase(it);  // erase current item and increment iterator
    myVector->erase(it);
    sem_post(sent_messages);
}

bool decipher_the_message(uint8_t *buf, int message_length) {
    std::cout << "Message accepted. Processing..." << std::endl;

    switch (buf[0]) {
        case 0x00://Confirm
            confirm_id_from_vector(buf);
            std::cout << "Confirm" << std::endl;
            //return false;
            break;
        case 0x01://REPLY
            delete_id_from_vector(buf);
            std::cout << "Reply" << std::endl;
            break;
        case 0x04://MSG
            read_msg_bytes(buf, message_length);
            return false;
            //break;
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
