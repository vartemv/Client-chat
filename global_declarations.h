//
// Created by artem on 3/13/24.
//
#include <list>
#include "vector"
#include "map"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <semaphore.h>
#include "Comm.h"
#include <sys/wait.h>
#include <algorithm>

#ifndef IPK_CPP_GLOBAL_QUEUES_H
#define IPK_CPP_GLOBAL_QUEUES_H

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::vector<char, CharAllocator> shm_vector;

bool handle_chat(std::string &userInput, SharedVector *myVector, shm_vector *vector_string, shm_vector *vector_display,
                 shm_vector *vector_channel);

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

sockaddr_in *server_address;
int client_socket;

#endif //IPK_CPP_GLOBAL_QUEUES_H
