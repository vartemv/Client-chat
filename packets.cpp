//
// Created by artem on 3/8/24.
//
#include <iostream>
#include "packets.h"

const char *TOKEN_IPK = "cc20830f-8124-49d9-b3d4-2d63dfe15bfb";

uint16_t bytes_to_uint16(const uint8_t* buf) {
    return (buf[1] << 8) | buf[2];
}

void increment_counter(){
    sem_wait(counter_stop);
    *count+=1;
    sem_post(counter_stop);
}

void auth_to_server(sockaddr_in server_address, int client_socket, std::string &u_n, std::string &disp_name) {
    uint8_t buf_out[256];
    //uint8_t buf_in[256];

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

bool decipher_the_message(uint8_t *buf, int message_length) {
    std::cout << "Message accepted. Processing..." << std::endl;

    switch (buf[0]) {
        case 0x00://Confirm
            std::cout << bytes_to_uint16(buf) <<std::endl;
            std::cout<<"confirm"<<std::endl;
            return false;
            //break;
        case 0x01://REPLY
            std::cout << "reply"<<std::endl;
            break;
        case 0x04://MSG
            std::cout << "msg";
            break;
        case 0xFE://ERR
            std::cout << "err";
            break;
        case 0xFF://BYE
            std::cout << "bye";
            return false;
        default:
            std::cout << "def";
            break;
    }
    return true;
}
