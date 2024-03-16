#include "Comm.h"
#include "global_declarations.h"
#include <sys/wait.h>

int main() {
    Init_values();
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    //auth_to_server(server_address, client_socket, str, str);

    std::string userInput;
    pid_t pid1 = fork();

    if (pid1 != 0) {
        while (*chat) {
            int ret = poll(fds, 1, 300);
            if (ret == -1) {
                printf("Error: poll failed\n");
            } else if (!ret) {
                continue;
            } else {
                std::getline(std::cin, userInput);
                if (!userInput.empty()) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        if (!handle_chat(userInput)) {
                            //end_connection_and_cleanup();
                            *chat = false;
                        }
                        userInput.clear();
                        kill(getpid(), SIGKILL);
                    }
                }
            }
        }
    } else {
        //listen_on_socket(server_address, client_socket);
    }

    //std::cout<<"exited"<<std::endl;
    while (wait(nullptr) > 0);

    //TODO Cleanup function
    close(client_socket);
    boost::interprocess::shared_memory_object::remove("shared_memory_for_bool");
    boost::interprocess::shared_memory_object::remove("MySharedMemory_VECTOR");
    //

    return 0;
}


bool handle_chat(std::string &userInput) {
    auto it = String_to_values.find(userInput);

    if (it != String_to_values.end()) {
        int value = it->second;

        if (!auth && value != evHelp) {
            std::cout << "You should sign in before doing anything" << std::endl;
        } else {
            switch (value) {
                case evAuth:
                    if (!*auth) {
                        *auth = true;
//                        auth_to_server(server_address, client_socket, );
                        std::cout << "authed" << std::endl;
                    } else {
                        std::cout << "You already authed to server" << std::endl;
                    }
                    break;
                case evJoin:
                    std::cout << "joined" << std::endl;
                    break;
                case evHelp:
                    std::cout << "help called" << std::endl;
                    break;
                case evEnd:
                    std::cout << "Exiting" << std::endl;
                    return false;
                case evRename:
                    std::cout << "renamed" << std::endl;
                    break;
            }
            myVector->push_back(count);
            count++;
        }
    } else {
        std::cout << "send msg" << std::endl;
        myVector->push_back(count);
        count++;
    }
    return true;
}

static void Init_values() {
    String_to_values["/auth"] = evAuth;
    String_to_values["/join"] = evJoin;
    String_to_values["/rename"] = evRename;
    String_to_values["/help"] = evHelp;
    String_to_values["exit"] = evEnd;

    chat = segment.construct<bool>("chat")(true);
    auth = segment.construct<bool>("auth")(false);
    open_state = segment.construct<bool>("open_state")(false);
    error = segment.construct<bool>("error")(false);
    end = segment.construct<bool>("end")(false);

    const ShmemAllocator alloc_inst(
            segment_for_vector.get_segment_manager()); // Initialize shared memory STL-compatible allocator
    myVector = segment_for_vector.construct<SharedVector>("MyVector")(
            alloc_inst); // Construct a vector named "MyVector" in shared memory with argument alloc_inst

    server_address = server_connection();
    client_socket = create_socket();
}