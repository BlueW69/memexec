#include <iostream>
#include "execmem.h"

int main()
{
    std::uint8_t code[]
    {
        0xB8, 0x2A, 0x00, 0x00, 0x00, // mov eax, 42
        0xC3                          // ret
    };

    execmem::rcg return_num;

    if (return_num.register_function(code) && return_num)
    {
        std::cout << return_num.call<int>();
    }

    std::cin.get();
}