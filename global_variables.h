//
// Created by artem on 3/13/24.
//

#include <list>
#include "packets.h"
#include "vector"
#include "map"

#ifndef IPK_CPP_GLOBAL_QUEUES_H
#define IPK_CPP_GLOBAL_QUEUES_H

std::list<int> id_of_sent_packets;
std::vector<std::string> user_input;
std::vector<Packet> messages_to_be_processed;

enum StringValue {
    evAuth,
    evJoin,
    evRename,
    evHelp,
    evEnd};

static std::map<std::string, StringValue> String_to_values;

#endif //IPK_CPP_GLOBAL_QUEUES_H
