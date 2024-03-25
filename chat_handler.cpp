//
// Created by artem on 3/20/24.
//


#include "chat_handler.h"

uint16_t port;
uint16_t timeout_chat;
uint8_t retransmissions;
bool UDP;
const char *HOST_chat;

bool get_parameters(int argc, char *argv[]) {
    port = 4567;
    timeout_chat = 250;
    retransmissions = 3;
    bool choosed_protocol = false;
    bool specified_host = false;

    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h") {
            std::cout << "This is help message" << std::endl;
            return false;
        } else if (arg == "-t") {
            i++;
            if (i < argc) {
                arg = argv[i];
                if (arg == "udp")
                    UDP = true;
                else if (arg == "tcp")
                    UDP = false;
                else {
                    std::cout << "Wrong argument for -t command" << std::endl;
                    return false;
                }
                choosed_protocol = true;
            } else {
                std::cout << "Nothing passed to protocol" << std::endl;
                return false;
            }
        } else if (arg == "-s") {
            i++;
            if (i < argc) {
                HOST_chat = argv[i];
            } else {
                std::cout << "Nothing passed to host" << std::endl;
                return false;
            }
            specified_host = true;
        } else if (arg == "-p") {
            i++;
            if (i < argc) {
                try {
                    port = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to port" << std::endl;
                    return false;
                }
            } else {
                std::cout << "Nothing passed to port" << std::endl;
                return false;
            }
        } else if (arg == "-d") {
            i++;
            if (i < argc) {
                try {
                    timeout_chat = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to timeout" << std::endl;
                    return false;
                }
            } else {
                std::cout << "Nothing passed to timeout" << std::endl;
                return false;
            }
        } else if (arg == "-r") {
            i++;
            if (i < argc) {
                try {
                    retransmissions = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to retransmissions" << std::endl;
                    return false;
                }
            } else {
                std::cout << "Nothing passed to retransmissions" << std::endl;
                return false;
            }
        }
    }

    if (!specified_host || !choosed_protocol) {
        std::cout << "You didnt specify host or protocol";
        return false;
    }

//    std::cout << HOST_chat << std::endl;
//    std::cout << port << std::endl;
//    std::cout << timeout_chat << std::endl;
//    std::cout << static_cast<int>(retransmissions) << std::endl;

    return true;
}