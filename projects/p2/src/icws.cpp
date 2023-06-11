#include <iostream>
#include <unistd.h>
#include <getopt.h>

const option long_opts[] =
{
    {"root", required_argument, nullptr, 'r'},
    {"port", required_argument, nullptr, 'p'},
    {nullptr, no_argument, nullptr, 0}
};

const char* root = nullptr;
const char* port = nullptr;

int process_args(int argc, char* argv[]){
    int opt;
    const char* short_opts = "";
    while((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1){
        switch(opt){
            case 'r':
                root = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                std::cerr << "Error: Invalid Option" << '\n';
                return EXIT_FAILURE;
        }
    }

    if(root == nullptr || port == nullptr){
        std::cerr << "Error: Missing required args" << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


int main(int argc, char* argv[]){
    if(process_args(argc, argv) == EXIT_FAILURE){
        return EXIT_FAILURE;
    };
    return 0;
}