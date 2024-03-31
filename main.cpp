#include "global_declarations.h"

sem_t *sent_messages;
sem_t *counter_stop;
sem_t *tcp_listening;
uint16_t *count;
bool *open_state;
bool *auth;
bool *listen_on_port;
bool *UDP;
SharedVector *local_vector;

void signalHandler(int signum) {
    say_bye(server_address, client_socket, local_vector);
    *chat = false;
}

/**
 * @brief This is the main function of the client.
 *
 * This function initializes and manages shared memory objects, creates and handles child processes, and performs
 * various cleanup tasks. It also contains the main logic of the software, including handling user input and
 * communication with a server.
 *
 * @param argc The number of command line arguments.
 * @param argv An array of command line argument strings.
 *
 * @return The exit status of the software.
 */
int main(int argc, char *argv[]) {

    boost::interprocess::shared_memory_object::remove("20");
    boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, "20", 1024);
    UDP = segment.construct<bool>("udp")(true);

    if (!get_parameters(argc, argv, UDP))
        return 0;

    int shm_fd = shm_open("MyShareMemorvalue", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(uint16_t));
    count = (uint16_t *) mmap(nullptr, sizeof(uint16_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *count = 0;

    int fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* Set the size of the shared memory object */
    if (ftruncate(fd, sizeof(sockaddr_in)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    /* Map the shared memory object into this process's address space */
    server_address = (sockaddr_in *) mmap(nullptr, sizeof(sockaddr_in),
                                          PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (server_address == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    boost::interprocess::shared_memory_object::remove("21");
    boost::interprocess::managed_shared_memory segment_for_vector(boost::interprocess::create_only, "21",
                                                                  65536);
    const ShmemAllocator alloc_inst(segment_for_vector.get_segment_manager());
    SharedVector *myVector = segment_for_vector.construct<SharedVector>("Myytor")(alloc_inst);
    local_vector = myVector;

    boost::interprocess::shared_memory_object::remove("UserName");
    boost::interprocess::managed_shared_memory segment_for_string_un(boost::interprocess::create_only, "UserName",
                                                                     1024);
    // Create vector in the segment with initializing allocator
    shm_vector *vector_UN = segment_for_string_un.construct<shm_vector>("SharedVector_UN")(
            segment_for_string_un.get_segment_manager());

    boost::interprocess::shared_memory_object::remove("DisplayName");
    boost::interprocess::managed_shared_memory segment_for_string(boost::interprocess::create_only, "DisplayName",
                                                                  1024);
    // Create vector in the segment with initializing allocator
    shm_vector *vector_DN = segment_for_string.construct<shm_vector>("SharedVector_DN")(
            segment_for_string.get_segment_manager());

    boost::interprocess::shared_memory_object::remove("Ch_ID");
    boost::interprocess::managed_shared_memory segment_for_channel(boost::interprocess::create_only, "Ch_ID", 1024);
    // Create vector in the segment with initializing allocator
    shm_vector *vector_CD = segment_for_channel.construct<shm_vector>("SharedVector_CD")(
            segment_for_channel.get_segment_manager());

    boost::interprocess::shared_memory_object::remove("117");
    boost::interprocess::managed_shared_memory segment_bool(boost::interprocess::create_only, "117", 1024);

    listen_on_port = segment_bool.construct<bool>("listening")(true);
    chat = segment.construct<bool>("chat")(true);
    auth = segment.construct<bool>("auth")(false);
    open_state = segment.construct<bool>("open_state")(false);

    if (!Init_values())
        return 1;

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = client_socket;

    *UDP ? fds[1].events = 0 : fds[1].events = POLLIN;

    std::string userInput;
    pid_t main_id = getpid();
    signal(SIGINT, signalHandler);

    if (*UDP) {
        pid_t pid1 = fork();
        if (pid1 == -1) {
            return 99;
        } else if (pid1 != 0) {
            while (*chat) {
                int ret = poll(fds, 1, 300);
                if (ret == -1) {
                    printf("Error occured or SIGINT was caught\n");
                } else if (!ret) {
                    continue;
                } else {
                    std::getline(std::cin, userInput);
                    if (!userInput.empty()) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            if (!handle_chat(userInput, myVector, vector_UN, vector_DN, vector_CD)) {
                                *chat = false;
                            }
                            userInput.clear();
                            break;
                        }
                    }
                }

            }
        } else {
            std::string DisplayName(vector_DN->begin(), vector_DN->end());
            if (!listen_on_socket(server_address, client_socket, myVector, DisplayName))
                *chat = false;
        }

    } else {
        uint8_t buf[4096];
        int len = sizeof(buf);
        if (!connect_tcp(client_socket, server_address))
            *chat = false;
        while (*chat) {
            int ret = poll(fds, 2, 300);
            if (ret > 0) {
                if (fds[0].revents && POLLIN) {//chat
                    std::getline(std::cin, userInput);
                    if (!userInput.empty()) {
                        if (!handle_chat(userInput, myVector, vector_UN, vector_DN, vector_CD)) {
                            *chat = false;
                        }
                        userInput.clear();
                    }
                }
                if (fds[1].revents && POLLIN) {//socket
                    std::string Name = "default";
                    if (!vector_DN->empty())
                        Name = std::string(vector_DN->begin(), vector_DN->end());
                    if (!receive_message_tcp(client_socket, buf, len, Name))
                        *chat = false;
                }
            } else if (ret == 0) {
                continue;
            } else {
                printf("Error occured or SIGINT was caught\n");
            }
        }
    }

    if (main_id == getpid()) {
        //TODO Cleanup function
        if (!*UDP) {
            shutdown(client_socket, SHUT_RDWR);
        }
        close(client_socket);
        boost::interprocess::shared_memory_object::remove("20");
        boost::interprocess::shared_memory_object::remove("21");
        segment_bool.destroy<bool>("listening");
        boost::interprocess::shared_memory_object::remove("117");
        segment_for_vector.destroy<SharedVector>("Myytor");
        segment.destroy<bool>("chat");
        segment.destroy<bool>("auth");
        segment.destroy<bool>("open_state");
        segment_for_vector.destroy_ptr(myVector);
        munmap(count, sizeof(uint16_t));
        close(shm_fd);
        if (munmap(server_address, sizeof(sockaddr_in)) == -1) {
            perror("Error un-mapping the shared memory segment");
            exit(EXIT_FAILURE);
        }
        if (shm_unlink("/my_shm") == -1) {
            perror("Error unlinking the shared memory segment");
            exit(EXIT_FAILURE);
        }

        // Additional cleaning
        boost::interprocess::shared_memory_object::remove("UserName");
        boost::interprocess::shared_memory_object::remove("DisplayName");
        boost::interprocess::shared_memory_object::remove("Ch_ID");
        segment_for_string_un.destroy_ptr(vector_UN);
        segment_for_string.destroy_ptr(vector_DN);
        segment_for_channel.destroy_ptr(vector_CD);

        sem_close(sent_messages);
        sem_close(counter_stop);
        sem_close(tcp_listening);
        shm_unlink("MyShareMemorvalue");
        sem_unlink("sent");
        sem_unlink("counterr");
        sem_unlink("tcp");
        close(fd);
    }
    while (wait(nullptr) > 0);
    return 0;
}

void write_to_vector(shm_vector *vector_string, std::string *source) {
    for (auto c: *source) {
        vector_string->push_back(c);
    }
}

void print_help(){
    std::cout <<
              R"(
/auth      {Username} {Secret} {DisplayName}__________________________________________
           Sends AUTH message with the data provided from the command to the server
           (and correctly handles the Reply message), locally sets the DisplayName
           value (same as the /rename command).


/join      {ChannelID}________________________________________________________________
           Sends JOIN message with channel name from the command to the server
           (and correctly handles the Reply message).

/rename    {DisplayName}______________________________________________________________
           Locally changes the display name of the user to be sent with new
           messages/selected commands.

/help      ___________________________________________________________________________
           Prints out supported local commands with their parameters and a description.
)";
}

/**
 * @brief Handles the chat command entered by the user.
 *
 * This function parses the user input and executes the corresponding action based on the command.
 * It also interacts with the server through various helper functions.
 *
 * @param[in] userInput The user input string.
 * @param[in] myVector Pointer to the SharedVector object.
 * @param[in] vector_string Pointer to the shm_vector object for string data.
 * @param[in] vector_display Pointer to the shm_vector object for display name data.
 * @param[in] vector_channel Pointer to the shm_vector object for channel ID data.
 *
 * @return Returns true to continue handling chat commands, or false to exit the chat application.
 */
bool handle_chat(std::string &userInput, SharedVector *myVector, shm_vector *vector_string, shm_vector *vector_display,
                 shm_vector *vector_channel) {

    std::istringstream iss(userInput);
    std::vector<std::string> result;
    for (std::string s; std::getline(iss, s, ' ');) {
        result.push_back(s);
    }
    auto it = String_to_values.find(result[0]);
    std::string UserName(vector_string->begin(), vector_string->end());
    std::string DisplayName(vector_display->begin(), vector_display->end());
    std::string ChannelID(vector_channel->begin(), vector_channel->end());


    if (it != String_to_values.end()) {
        int value = it->second;
        if (!*auth) {
            if (value != evAuth)
                if (value != evHelp) {
                    if (value != evEnd) {
                        std::cerr << "ERR: You have to sign in before doing anything" << std::endl;
                        return true;
                    }
                }
        }
        switch (value) {
            case evAuth:
                if (result.size() < 4) {
                    std::cerr << "ERR: format is /auth {Username} {Secret} {DisplayName}" << std::endl;
                    break;
                }
                if (!*auth) {
                    if (result[1].length() > 20) {
                        std::cerr << "ERR: Username shouldn't be longer than 20 characters";
                        break;
                    }
                    if (result[2].length() > 128) {
                        std::cerr << "ERR: Secret shouldn't be longer than 128 characters";
                        break;
                    }
                    if (result[3].length() > 20) {
                        std::cerr << "ERR: DisplayName shouldn't be longer than 20 characters";
                        break;
                    }
                    write_to_vector(vector_string, &result[1]);
                    write_to_vector(vector_display, &result[3]);
                    auth_to_server(server_address, client_socket, result[1], result[3], result[2], myVector);
                } else {
                    std::cerr << "ERR: You already authed to server" << std::endl;
                }
                break;
            case evJoin:
                if (result.size() < 2) {
                    std::cerr << "ERR: format is /join {ChannelID}" << std::endl;
                    break;
                }
                if (result[1].length() > 20) {
                    std::cerr << "ERR: Channel ID shouldn't be longer than 20 characters";
                    break;
                }
                join_to_server(server_address, client_socket, result[1], DisplayName, myVector);
                break;
            case evHelp:
                  print_help();
                break;
            case evEnd:
                if (say_bye(server_address, client_socket, myVector))
                    return false;
                break;
            case evRename:
                if (result.size() < 2) {
                    std::cerr << "ERR: Format is /rename {DisplayName}" << std::endl;
                    break;
                }
                if (result[1].length() > 20) {
                    std::cerr << "ERR: DisplayName shouldn't be longer than 20 characters";
                    break;
                }
                vector_display->clear();
                write_to_vector(vector_display, &result[1]);
                std::cout << "Renamed" << std::endl;
                break;
        }
    } else {
        //Message sending
        if (!*auth) {
            std::cerr << "ERR: Sign in before doing anything" << std::endl;
        } else {
            send_msg(server_address, client_socket, DisplayName, userInput, false, myVector);
        }

    }
    return true;
}

/**
 * @brief Initializes the values for the string to enum mapping, semaphore creation, and socket creation
 * @return true if successful, false otherwise
 */
static bool Init_values() {
    String_to_values["/auth"] = evAuth;
    String_to_values["/join"] = evJoin;
    String_to_values["/rename"] = evRename;
    String_to_values["/help"] = evHelp;
    String_to_values["/exit"] = evEnd;

    if (((sent_messages = sem_open("sent", O_CREAT | O_WRONLY, 0666, 1)) == SEM_FAILED) ||
        ((counter_stop = sem_open("counterr", O_CREAT | O_WRONLY, 0666, 1)) == SEM_FAILED) ||
        (tcp_listening = sem_open("tcp", O_CREAT | O_WRONLY, 0666, 1)) == SEM_FAILED) {
        perror("sem_open");
        return false;
    }

    sem_post(sent_messages);

    server_connection(server_address);
    client_socket = create_socket();
    return true;
}