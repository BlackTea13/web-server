#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

int main(){
    std::cout << "Hello World!" << std::endl;

    char* a = (char*) malloc(sizeof(int) * 10);

    std::unique_ptr<char> b;
    b.reset(a);

    char buf[5000];
    memset(buf, 0, 5000);

    return 0;
}