//
// Created by artem on 2/25/24.
//

#ifndef IPK_CPP_PACKETS_H
#define IPK_CPP_PACKETS_H

#include <string>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <semaphore.h>
#include <chrono>
#include <thread>
#include "chat_handler.h"

typedef boost::interprocess::allocator<uint16_t, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocator;
typedef boost::interprocess::vector<uint16_t, ShmemAllocator> SharedVector;

void auth_to_server(sockaddr_in* server_address, int client_socket, std::string &u_n, std::string &disp_name, SharedVector *myVector);

void say_bye(sockaddr_in* server_address, int client_socket, SharedVector *myVector);

void send_confirm(sockaddr_in* server_address, int client_socket, int ref_id);

void join_to_server(sockaddr_in* server_address, int client_socket, std::string &ch_id, std::string &disp_name, SharedVector *myVector);

void send_msg(sockaddr_in* server_address, int client_socket, std::string &disp_name, std::string &msg, bool error, SharedVector *myVector);

bool decipher_the_message(uint8_t *buf, int message_length, SharedVector *myVector, sockaddr_in* server_address, int client_socket);

void increment_counter();

extern bool *auth;
extern bool *open_state;
extern bool *error;
extern bool *end;
extern bool *listen_on_port;


//extern SharedVector *myVector;
extern sem_t *sent_messages;
extern sem_t *counter_stop;
extern uint16_t *count;
//extern uint16_t *count;

#endif //IPK_CPP_PACKETS_H

//To create BYE use Packets struct
typedef struct Packets {
    uint8_t MessageType;
    uint16_t MessageID;

    Packets(uint8_t type, uint16_t id) {
        MessageType = type;
        MessageID = id;
    }

    virtual int construct_message(uint8_t *b) {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);


        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);
        return 3;
    }

} Packet;

typedef struct ConfirmPackets : public Packets {

    uint16_t Ref_MessageID;

    ConfirmPackets(uint8_t type, uint16_t id, uint16_t ref_id) : Packets(type, id) {
        Ref_MessageID = ref_id;
    }
    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        uint16_t ID = this->Ref_MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);
        return sizeof(this->MessageType)+sizeof (ID);
    }

} ConfirmPacket;

typedef struct ReplyPackets : public Packets {

    int8_t Result;
    uint16_t Ref_MessageID;
    const char *MessageContents;

    ReplyPackets(uint8_t type, uint16_t id, int8_t result, uint16_t ref_id, const char *content) : Packets(type, id) {
        Result = result;
        Ref_MessageID = ref_id;
        MessageContents = content;
    }

} ReplyPacket;

typedef struct JoinPackets : public Packets {

    std::string ChannelID;
    std::string DisplayName;

    JoinPackets(uint8_t type, uint16_t id, std::string ch_id, std::string disp_name) : Packets(type, id) {
        ChannelID = std::move(ch_id);
        DisplayName = std::move(disp_name);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, ChannelID.c_str(), ChannelID.length());
        b[ChannelID.length()] = '\0';
        b += ChannelID.length() + 1;

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + ChannelID.length() + 1 + DisplayName.length() + 1;
    }

} JoinPacket;

// To create ERR use MsgPackets struct
typedef struct MsgPackets : public Packets {

    std::string MessageContents;
    std::string DisplayName;

    MsgPackets(uint8_t type, uint16_t id, std::string content, std::string disp_name) : Packets(type, id) {
        MessageContents = std::move(content);
        DisplayName = std::move(disp_name);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;

        memcpy(b, MessageContents.c_str(), MessageContents.length());
        b[MessageContents.length()] = '\0';
        b += MessageContents.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + DisplayName.length() + 1 + MessageContents.length() + 1;
    }

} MsgPacket;

typedef struct AuthPackets : public Packets {

    std::string Username;
    std::string DisplayName;
    std::string Secret;

    AuthPackets(uint8_t type, uint16_t id, std::string u_n, std::string disp_name, std::string sec) : Packets(type,
                                                                                                              id) {
        Username = std::move(u_n);
        DisplayName = std::move(disp_name);
        Secret = std::move(sec);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);
        //std::cout<<this->MessageType<<std::endl;

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, Username.c_str(), Username.length());
        b[Username.length()] = '\0';
        b += Username.length() + 1;

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;

        memcpy(b, Secret.c_str(), Secret.length());
        b[Secret.length()] = '\0';
        b += Secret.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + Username.length() + 1 + DisplayName.length() + 1 +
               Secret.length() + 1;
    }

} AuthPacket;