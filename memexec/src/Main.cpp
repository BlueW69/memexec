#include <iostream>
#include "memexec.h"

int main()
{
    // mov eax, [esp+4] ; Load the first argument from the stack into EAX
    // add eax, eax     ; Double it
    // ret 4            ; Return and pop 4 bytes(the int) off the stack(stdcall cleanup)
    std::uint8_t code[] = { 0x8B, 0xC1, 0x03, 0xC0, 0xC3 };

    std::vector<memexec::type> types { memexec::type::i32};
    std::vector<memexec::value> values { 8 };

    memexec::rfie return_num;

    if (return_num.register_function(code) && return_num)
    {
        memexec::rfie::function_structure str { };
        str.call_conv = memexec::callconv::stdcall;
        str.return_type = memexec::type::i32;
        str.arguments_types = types;
        str.arguments_values = values;

        memexec::value val = return_num.call(str);

        std::cout << val.i32;
    }

    std::cin.get();
}