#include "Comm.h"
#include "global_variables.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

bool handle_chat(std::string& userInput);
static void Init_values();

int main() {

    Init_values();
    sockaddr_in server_address = server_connection();
    int client_socket = create_socket();
    std::string test = "test";
    std::string user_name = "xveren00";

//    using namespace boost::interprocess;
    //Typedefs
    typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager>
            CharAllocator;
    typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator>
            MyShmString;
    typedef boost::interprocess::allocator<MyShmString, boost::interprocess::managed_shared_memory::segment_manager>
            StringAllocator;
    typedef boost::interprocess::vector<MyShmString, StringAllocator>
            MyShmStringVector;

    //Open shared memory
    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { boost::interprocess::shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove(){ boost::interprocess::shared_memory_object::remove("MySharedMemory"); }
    } remover;

    boost::interprocess::managed_shared_memory shm(boost::interprocess::create_only, "MySharedMemory", 10000);

    //Create allocators
    CharAllocator     charallocator  (shm.get_segment_manager());
    StringAllocator   stringallocator(shm.get_segment_manager());

    //This string is in only in this process (the pointer pointing to the
    //buffer that will hold the text is not in shared memory).
    //But the buffer that will hold "this is my text" is allocated from
    //shared memory
    MyShmString mystring(charallocator);

    //This vector is fully constructed in shared memory. All pointers
    //buffers are constructed in the same shared memory segment
    //This vector can be safely accessed from other processes.
    MyShmStringVector *myshmvector =
            shm.construct<MyShmStringVector>("myshmvector")(stringallocator);

    std::string userInput;
    while(1){
        std::getline(std::cin, userInput);

        mystring = userInput;
        myshmvector->insert(myshmvector->begin(), 1, mystring);

        if(!handle_chat(userInput))
            break;

        std::cout<<myshmvector[0][0]<<std::endl;
    }

    //Destroy vector. This will free all strings that the vector contains
    shm.destroy_ptr(myshmvector);

    return 0;
}

bool handle_chat(std::string& userInput){
    switch (String_to_values[userInput]) {
        case evAuth:
            return false;
        case evJoin:
            return false;
        case evHelp:
            return false;
        case evEnd:
            return false;
        case evRename:
            return false;
        default:
            return false;
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