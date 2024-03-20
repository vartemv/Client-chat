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

bool waiting_for_confirm(int counter, int client_socket, uint8_t *buf_out, int len, int address_size,
                         sockaddr_in server_address, SharedVector *myVector) {
    long bytes_tx;
    for (int i = 0; i < retransmissions; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_chat));
        sem_wait(sent_messages);
        auto it = std::find(myVector->begin(), myVector->end(), counter);
        sem_post(sent_messages);
        if (it != myVector->end()) {
            if (std::next(it) != myVector->end() && *std::next(it) != 1) {
                std::cout << "Not confirmed, sending again" << std::endl;
                bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address,
                                  address_size);
                if (bytes_tx < 0) perror("ERROR: sendto");
            } else if (*std::next(it) == 1) {
                std::cout << "Message confirmed" << std::endl;
                return true;
            }
        }else{
            return true;
        }
    }
    return false;
}

void add_to_sent_messages(SharedVector *myVector, int counter){
    sem_wait(sent_messages);
    myVector->push_back(counter);
    myVector->push_back(0);
    sem_post(sent_messages);

    auto it1 = std::find(myVector->begin(), myVector->end(), 65535);
    auto it2 = std::find(myVector->begin(), myVector->end(), 65266);
    if (it1 != myVector->end()) {
        *it1 = 0;
    }
    if (it2 != myVector->end()) {
        *it2 = 0;
    }
}

void auth_to_server(sockaddr_in server_address, int client_socket, std::string &u_n, std::string &disp_name,
                    SharedVector *myVector) {
    uint8_t buf_out[256];
    //uint8_t buf_in[256];

    int local_count = *count;
    AuthPacket authPacket(0x02, *count, u_n, disp_name, TOKEN_IPK);
//    *count += 1;
    increment_counter();

    int len = authPacket.construct_message(buf_out);

    printf("INFO: Server socket: %s : %d \n",
           inet_ntoa(server_address.sin_addr),
           ntohs(server_address.sin_port));

    int address_size = sizeof(server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address,
                           address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    add_to_sent_messages(myVector, local_count);

    if(!waiting_for_confirm(local_count, client_socket, buf_out, len, address_size, server_address, myVector))
        std::cout << "Couldn't auth to server, try again"<<std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    (!*auth) ? std::cout<<"Auth not succesfull, try again"<<std::endl : std::cout<<"Auth succesful"<<std::endl;
}

void say_bye(sockaddr_in server_address, int client_socket, SharedVector *myVector) {

    uint8_t buf[3];

    int local_counter = *count;
    Packets bye(0xFF, *count);
    increment_counter();

    int len = bye.construct_message(buf);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");



    add_to_sent_messages(myVector, local_counter);

}

void join_to_server(sockaddr_in server_address, int client_socket, std::string &ch_id, std::string &d_name,
                    SharedVector *myVector) {
    uint8_t buf[256];

    JoinPacket join(0x03, *count, ch_id, d_name);
    increment_counter();

    int len = join.construct_message(buf);

    long bytes_tx = sendto(client_socket, buf, len, 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (bytes_tx < 0) perror("ERROR: sendto");
}

void send_msg(sockaddr_in server_address, int client_socket, std::string &disp_name, std::string &msg, bool error,
              SharedVector *myVector) {
    uint8_t buf_out[1250];

    int local_count = *count;

    MsgPacket message(error ? 0xFE : 0x04, *count, msg, disp_name);
    increment_counter();

    int len = message.construct_message(buf_out);

    int address_size = sizeof(server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address, address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    add_to_sent_messages(myVector, local_count);


    waiting_for_confirm(local_count, client_socket, buf_out, len, address_size, server_address, myVector);

}

void send_confirm(sockaddr_in server_address, int client_socket, int ref_id){
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, *count, ref_id);

    int len = confirm.construct_message(buf_out);

    int address_size = sizeof(server_address);

    long bytes_tx = sendto(client_socket, buf_out, len, 0, (struct sockaddr *) &server_address, address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");
}

int read_packet_id(uint8_t *buf){
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
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

void identify_reply_type(uint8_t *buf){
    int result = buf[3];
    //std::cout << "result is "<<result << std::endl;
    if(!*open_state){
        //std::cout << "result is "<<result << std::endl;
        (!result) ? *auth = false : *auth = true;
    }
}

void confirm_id_from_vector(uint8_t *buf, SharedVector *myVector) {
    int result = buf[1] << 8 | buf[2];

    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    if (it != myVector->end()) {
        *std::next(it) = 1;
    }else{
        //std::cout<<"Dont exist confirm " << result << std::endl;
    }
    sem_post(sent_messages);
}

void delete_id_from_vector(uint8_t *buf, SharedVector *myVector, sockaddr_in server_address, int client_socket) {

    int result = buf[4] << 8 | buf[5];
    //std::cout<<"RefId is "<<result<<std::endl;
    result = ntohs(result);
    sem_wait(sent_messages);
    auto it = std::find(myVector->begin(), myVector->end(), result);
    if (it != myVector->end()) {
//        for(const auto &item : *myVector) {
//            std::cout << item << ' ';
//        }
//        std::cout << '\n';
        it = myVector->erase(it);  // erase current item and increment iterator
        myVector->erase(it);
        send_confirm(server_address, client_socket,read_packet_id(buf));
//        for(const auto &item : *myVector) {
//            std::cout << item << ' ';
//        }
//        std::cout << '\n';
    }
    sem_post(sent_messages);
}

bool decipher_the_message(uint8_t *buf, int message_length, SharedVector *myVector, sockaddr_in server_address, int client_socket) {
    //std::cout << "Message accepted. Processing..." << std::endl;

    switch (buf[0]) {
        case 0x00://Confirm
            confirm_id_from_vector(buf, myVector);
            std::cout << "Confirm" << std::endl;
            //return false;
            break;
        case 0x01://REPLY
            identify_reply_type(buf);
            delete_id_from_vector(buf, myVector, server_address, client_socket);
            //std::cout << "Reply" << std::endl;
            break;
        case 0x04://MSG
            read_msg_bytes(buf, message_length);
            send_confirm(server_address, client_socket,read_packet_id(buf));//read_packet_id(buf);
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
