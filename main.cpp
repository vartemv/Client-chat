#include "Comm.h"
#include "global_variables.h"
#include <sys/wait.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <poll.h>

bool handle_chat(std::string& userInput);
static void Init_values();

int main() {

    Init_values();
    sockaddr_in server_address = server_connection();
    int client_socket = create_socket();

    struct shm_remove {
        shm_remove() { boost::interprocess::shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove(){ boost::interprocess::shared_memory_object::remove("MySharedMemory"); }
    } remover;

    // Create a new segment with given name and size
    boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, "MySharedMemory", 1024);

    // Initialize shared memory bool
    bool* myBool = segment.construct<bool>("myBool")(false);

    // Now let's change the value of the bool to true
    *myBool = true;
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    std::string userInput;
    pid_t pid1 = fork();
    if(pid1 != 0){
    while(*myBool){
        int ret = poll(fds, 1, 300);
        if(ret == -1){
            printf("Error: poll failed\n");
        }else if (!ret){
            continue;
        }
        else{
            std::getline(std::cin, userInput);
            if (!userInput.empty()){
                pid_t pid = fork();
                if (pid == 0){
                    if (!handle_chat(userInput)) {
                     //end_connection_and_cleanup();
                        *myBool= false;}
                    std::cout<<userInput<<std::endl;
                    userInput.clear();
                    kill(getpid(), SIGKILL);}
            }
        }
    }
    }else{
        while(true){
            //listen_on_socket();
            break;
        }
    }
    while(wait(NULL)>0);
    return 0;
}



bool handle_chat(std::string& userInput){
    switch (String_to_values[userInput]) {
        case evAuth:

            return true;
        case evJoin:
            return true;
        case evHelp:
            return true;
        case evEnd:
            return false;
        case evRename:
            return true;
        default:
            return true;
    }
    return true;

}

static void Init_values(){
    String_to_values["/auth"] = evAuth;
    String_to_values["/join"] = evJoin;
    String_to_values["/rename"] = evRename;
    String_to_values["/help"] = evHelp;
    String_to_values["exit"] = evEnd;
}