//
// Created by artem on 3/13/24.
//

#include <list>
#include "vector"
#include "map"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>

#ifndef IPK_CPP_GLOBAL_QUEUES_H
#define IPK_CPP_GLOBAL_QUEUES_H

bool handle_chat(std::string &userInput);

static void Init_values();

enum StringValue {
    evAuth,
    evJoin,
    evRename,
    evHelp,
    evEnd
};

static std::map<std::string, StringValue> String_to_values;

struct shm_remove {
    shm_remove() { boost::interprocess::shared_memory_object::remove("shared_memory_for_bool"); }

    ~shm_remove() { boost::interprocess::shared_memory_object::remove("shared_memory_for_bool"); }
} remover;

// Create a new segment with given name and size
boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, "shared_memory_for_bool", 1024);

typedef boost::interprocess::allocator<uint16_t, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocator;
typedef boost::interprocess::vector<uint16_t, ShmemAllocator> SharedVector;
boost::interprocess::managed_shared_memory segment_for_vector(boost::interprocess::create_only,
                                                              "MySharedMemory_VECTOR",
                                                              65536);
SharedVector *myVector;

bool *chat;
bool *auth;
bool *open_state;
bool *error;
bool *end;

sockaddr_in server_address;
int client_socket;
uint16_t count = 0;

#endif //IPK_CPP_GLOBAL_QUEUES_H
