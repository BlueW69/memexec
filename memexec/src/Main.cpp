#include <iostream>
#include "memexec.h"

int main()
{
    // mov eax, [esp+4] ; Load the first argument from the stack into EAX
    // add eax, eax     ; Double it
    // ret 4            ; Return and pop 4 bytes(the int) off the stack(stdcall cleanup)
    std::uint8_t code[] = { 0x8B, 0xC1, 0x03, 0xC0, 0xC3 };

    std::vector<memexec::type> types{ memexec::string_to_type("i32").value()};
    std::vector<memexec::value> values { memexec::string_to_value("3", types[0]).value()};

    memexec::rfie::function_structure str{ };
    str.call_conv = memexec::string_to_callconv("stdcall").value();
    str.return_type = types[0];
    str.arguments_types = types;
    str.arguments_values = values;
    
    try
    {
        memexec::rfie return_num(code, str); 
        
        if (return_num)
        {
            memexec::value val = return_num.call();

            std::cout << val.i32;
        }

        std::cin.get();
    }
    catch(...)
    {
        std::cout << "whoopsie";
    }
}