#include <iostream>

#define CVK_IMPLEMENTATION
#include "cvk.hpp"


template<typename... Args>
void g(int a, Args&& ...args)
{
    std::cout << "A: " << a << "\n";
}


template<typename... Args>
void f(Args&& ...args)
{
    g(args...);
    std::cout << "Hello, World\n";
}


int main()
{
    cvk::VkIns::init("Hello");
    return 0;
}
