//
// Created by artem on 3/20/24.
//

#ifndef IPK_CPP_CHAT_HANDLER_H
#define IPK_CPP_CHAT_HANDLER_H

#include <cstdint>
#include <string>
#include <iostream>

extern const char *HOST_chat;
extern uint16_t port;
extern uint16_t timeout_chat;
extern uint8_t retransmissions;

bool get_parameters(int argc, char *argv[], bool *UDP);

#endif //IPK_CPP_CHAT_HANDLER_H
