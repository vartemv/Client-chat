#include "Comm.h"
#include "global_declarations.h"
#include <sys/wait.h>
#include <algorithm>

//Declaration of shared memory for vector and semaphores
//
sem_t *sent_messages;
sem_t *counter_stop;
uint16_t *count;

int main() {

    int shm_fd = shm_open("MyShareMemorvalue", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(uint16_t));
    count = (uint16_t *) mmap(0, sizeof(uint16_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *count = 0;

//    pid_t pid13 = fork();
//
//    if (pid13 == 0){
//        pid_t pid14 = fork();
//        if (pid14 == 0){
//            for(int i = 0; i<5; i++){
//                int pid15 = fork();
//                if(pid15 == 0){
//                    myVector->push_back(*count);
//                    myVector->push_back(0);
//                    *count+=1;
//                    for(const uint16_t &item : *myVector) {
//                        std::cout << item << ' ';
//                    }
//                    std::cout << std::endl;
//                }else{
//                    break;
//                }
//            }
//        }
//    }
//
//
//    while (wait(nullptr) > 0);
//    return 1;
    boost::interprocess::shared_memory_object::remove("21");
    boost::interprocess::managed_shared_memory segment_for_vector(boost::interprocess::create_only, "21",
                                                                  65536);
    const ShmemAllocator alloc_inst(segment_for_vector.get_segment_manager());
    SharedVector *myVector = segment_for_vector.construct<SharedVector>("Myytor")(alloc_inst);

    boost::interprocess::shared_memory_object::remove("20");
    boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, "20", 1024);
    chat = segment.construct<bool>("chat")(true);
    auth = segment.construct<bool>("auth")(false);
    open_state = segment.construct<bool>("open_state")(false);
    error = segment.construct<bool>("error")(false);
    end = segment.construct<bool>("end")(false);

    if (!Init_values())
        return 1;

    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    std::string userInput;

    pid_t pid1 = fork();
    if (pid1 == -1) {
        return 99;
    } else if (pid1 != 0) {
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
                        if (!handle_chat(userInput, myVector)) {
                            //end_connection_and_cleanup();
                            *chat = false;
                        }
                        userInput.clear();
                        //std::cout << "helper chat exited" << std::endl;
                        break;
                        //kill(getpid(), SIGKILL);
                    }
                }
            }

        }
    } else {
        listen_on_socket(server_address, client_socket, myVector);
    }

//    if (pid1 == 0)
//        std::cout << "socket exited" << std::endl;

    while (wait(nullptr) > 0);

    //TODO Cleanup function
    close(client_socket);
    boost::interprocess::shared_memory_object::remove("20");
    boost::interprocess::shared_memory_object::remove("21");
    segment_for_vector.destroy<SharedVector>("Myytor");
    segment.destroy<bool>("chat");
    segment.destroy<bool>("auth");
    segment.destroy<bool>("open_state");
    segment.destroy<bool>("error");
    segment.destroy<bool>("end");
    segment_for_vector.destroy_ptr(myVector);
    munmap(count, sizeof(uint16_t));
    close(shm_fd);
    shm_unlink("MyShareMemorvalue");
    sem_unlink("sent");
    sem_unlink("counterr");
    sem_close(sent_messages);
    sem_close(counter_stop);
    //
    return 0;
}


bool handle_chat(std::string &userInput, SharedVector *myVector) {
    auto it = String_to_values.find(userInput);
    std::string test = "xveren00";

    if (it != String_to_values.end()) {
        int value = it->second;

        if (!auth && value != evHelp) {
            std::cout << "You should sign in before doing anything" << std::endl;
        } else {

            sem_wait(sent_messages);
            myVector->push_back(*count);
            myVector->push_back(0);
            sem_post(sent_messages);

            auto it1 = std::find(myVector->begin(), myVector->end(), 65535);
            auto it2 = std::find(myVector->begin(), myVector->end(), 65266);
            if (it1 != myVector->end()) {
                *it1 = 0;
            }
            if (it2 != myVector->end()) {
                *it2 = 0;
            }

            switch (value) {
                case evAuth:
                    if (!*auth) {
                        *auth = true;
                        auth_to_server(server_address, client_socket, test, test, myVector);
                        std::cout << "authed" << std::endl;
                    } else {
                        std::cout << "You already authed to server" << std::endl;
                        auth_to_server(server_address, client_socket, test, test, myVector);
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
        }
    } else {
        if (!auth) {
            std::cout << "You should sign in before doing anything" << std::endl;
        } else {
            sem_wait(sent_messages);
            myVector->push_back(*count);
            myVector->push_back(0);
            sem_post(sent_messages);

            auto it1 = std::find(myVector->begin(), myVector->end(), 65535);
            auto it2 = std::find(myVector->begin(), myVector->end(), 65266);
            if (it1 != myVector->end()) {
                *it1 = 0;
            }
            if (it2 != myVector->end()) {
                *it2 = 0;
            }

            send_msg(server_address, client_socket, test, userInput, false, myVector);
            std::cout << "send msg" << std::endl;
        }
    }
    return true;
}

static bool Init_values() {
    String_to_values["/auth"] = evAuth;
    String_to_values["/join"] = evJoin;
    String_to_values["/rename"] = evRename;
    String_to_values["/help"] = evHelp;
    String_to_values["exit"] = evEnd;

//    boost::interprocess::shared_memory_object::remove("20");
//    boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, "20", 1024);
//    chat = segment.construct<bool>("chat")(true);
//    auth = segment.construct<bool>("auth")(false);
//    open_state = segment.construct<bool>("open_state")(false);
//    error = segment.construct<bool>("error")(false);
//    end = segment.construct<bool>("end")(false);

    if (((sent_messages = sem_open("sent", O_CREAT | O_WRONLY, 0666, 1)) == SEM_FAILED) ||
        ((counter_stop = sem_open("counterr", O_CREAT | O_WRONLY, 0666, 1)) == SEM_FAILED)) {
        perror("sem_open");
        return false;
    }

    server_address = server_connection();
    client_socket = create_socket();
    return true;
}