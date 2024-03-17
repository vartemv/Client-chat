//
// Created by artem on 3/13/24.
//

#include <list>
#include "vector"
#include "map"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <semaphore.h>


#ifndef IPK_CPP_GLOBAL_QUEUES_H
#define IPK_CPP_GLOBAL_QUEUES_H

bool handle_chat(std::string &userInput);

static bool Init_values();

enum StringValue {
    evAuth,
    evJoin,
    evRename,
    evHelp,
    evEnd
};

static std::map<std::string, StringValue> String_to_values;
bool *chat;
bool *auth;
bool *open_state;
bool *error;
bool *end;

sockaddr_in server_address;
int client_socket;

#endif //IPK_CPP_GLOBAL_QUEUES_H
