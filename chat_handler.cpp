//
// Created by artem on 3/20/24.
//
#include "chat_handler.h"

uint16_t port;
uint16_t timeout_chat;
uint8_t retransmissions;
const char *HOST_chat;

void print_help_cli() {
    std::cout <<
              R"(| -t    | User provided | tcp or udp                | Transport protocol used for connection      |
| -s    | User provided | IP address or hostname    | Server IP or hostname                       |
| -p    | 4567          | uint16                    | Server port                                 |
| -d    | 250           | uint16                    | UDP confirmation timeout                    |
| -r    | 3             | uint8                     | Maximum number of UDP retransmissions       |
)";
}

/**
 * @brief Parses command line arguments and retrieves the necessary parameters.
 *
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @param UDP A pointer to a boolean that indicates if UDP protocol should be used.
 * @return bool Returns true if the parameters were successfully retrieved, false otherwise.
 *
 * This function parses the command line arguments and retrieves the necessary parameters, such as the transport protocol,
 * server IP address or hostname, server port, timeout value, and number of retransmissions. It also sets the value of the UDP
 * parameter based on the passed arguments. The function returns true if all the required parameters were provided, and false
 * otherwise.
 *
 * Command line arguments:
 *
 * -t: Specifies the transport protocol (TCP or UDP)
 *    - Valid values: "tcp", "udp"
 *
 * -s: Specifies the server IP address or hostname
 *    - Valid value: Any valid IP address or hostname
 *
 * -p: Specifies the server port
 *    - Valid value: An integer value within the range of uint16_t
 *    - Default value: 4567
 *
 * -d: Specifies the timeout value for UDP confirmation
 *    - Valid value: An integer value within the range of uint16_t
 *    - Default value: 250
 *
 * -r: Specifies the maximum number of UDP retransmissions
 *    - Valid value: An integer value within the range of uint8_t
 *    - Default value: 3
 *
 */
bool get_parameters(int argc, char *argv[], bool *UDP) {
    port = 4567;
    timeout_chat = 250;
    retransmissions = 3;
    bool choosed_protocol = false;
    bool specified_host = false;

    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h") {
            print_help_cli();
            return false;
        } else if (arg == "-t") {
            i++;
            if (i < argc) {
                arg = argv[i];
                if (arg == "udp")
                    *UDP = true;
                else if (arg == "tcp")
                    *UDP = false;
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
    return true;
}