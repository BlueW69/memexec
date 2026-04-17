#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <oleauto.h>
#include <stdexcept>
#include <vector>
#include <span>

#pragma comment(lib, "oleaut32.lib")

namespace memexec
{
    enum class types : std::uint8_t
    {
        // ------------------------- Special Cases ------------------------- //

        none = 0,         // VT_EMPTY        (void)
        void_ptr,         // byref           (PVOID)
        boolean,          // boolVal         (VARIANT_BOOL)    

        // ------------------------ Signed Integers ------------------------ //

        i8,               // cVal            (char)
        i16,              // iVal            (short)
        i32,              // intVal | lVal   (int) | (long)
        i64,              // llVal           (long long)
                                     
        // ----------------------- Unsigned Integers ----------------------- //

        u8,               // bVal            (byte)
        u16,              // uiVal           (unsigned short)
        u32,              // uintVal | ulVal (unsigned int) | (unsigned long)
        u64,              // ullVal          (unsigned long long)
                                     
        // ------------------------ Floating Points ------------------------ // 

        f32,              // fltVal          (FLOAT)
        f64               // dblVal          (DOUBLE)

        // ----------------------------------------------------------------- //
    };

    struct value
    {

    };

    class rcg
    {
    private:

        void* mem_ = nullptr;
        size_t size_ = 0;
        
        void cleanup()
        {
            if (mem_)
            {
                VirtualFree(mem_, 0, MEM_RELEASE);
                mem_ = nullptr;
                size_ = 0;
            }
        }

    public:

        

        struct function_structure
        {
            types return_type { };
            std::vector<types> argument_types { };
            std::vector<VARIANTARG*> argument_values { };
        };

        rcg() {}

        rcg(const std::uint8_t* code, size_t size)
        {
            mem_ = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            if (!mem_)
            {
                throw std::bad_alloc();
            }

            std::memcpy(mem_, code, size);
            size_ = size;

            DWORD discard;
            if (!VirtualProtect(mem_, size, PAGE_EXECUTE_READ, &discard))
            {
                cleanup();
                throw std::runtime_error("Failed to set execute rights.");
            }
        }

        rcg(std::span<std::uint8_t> code) : rcg(code.data(), code.size()) {}

        rcg(const rcg&) = delete;
        rcg& operator=(const rcg&) = delete;

        rcg(rcg&& other) noexcept : mem_(other.mem_), size_(other.size_)
        {
            other.mem_ = nullptr;
            other.size_ = 0;
        }

        rcg& operator=(rcg&& other) noexcept
        {
            if (this != &other)
            {
                cleanup();

                mem_ = other.mem_;
                size_ = other.size_;
                other.mem_ = nullptr;
                other.size_ = 0;
            }

            return *this;
        }

        ~rcg()
        {
            cleanup();
        }


        bool register_function(const std::uint8_t* code, size_t size) noexcept
        {
            cleanup();

            mem_ = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            if (!mem_)
            {
                return false;
            }

            std::memcpy(mem_, code, size);
            size_ = size;

            DWORD discard;
            if (!VirtualProtect(mem_, size, PAGE_EXECUTE_READ, &discard))
            {
                cleanup();
                return false;
            }

            return true;
        }

        bool register_function(std::span<std::uint8_t> code) noexcept
        {
            return register_function(code.data(), code.size());
        }


        template <typename ReturnType, typename... ParamTypes>
        auto get() const noexcept -> ReturnType(*)(ParamTypes...)
        {
            return reinterpret_cast<ReturnType(*)(ParamTypes...)>(mem_);
        }

        template <typename ReturnType, typename... Params>
        ReturnType call(Params&&... args) const noexcept
        {
            return get<ReturnType, Params...>()(std::forward<Params>(args)...);
        }


        #if defined(_M_X64) || defined(__x86_64__)

        //VARIANT assemble_and_call(function_structure& str)
        //{
        //    VARIANT result;
        //
        //    DispCallFunc(nullptr, 
        //                 reinterpret_cast<ULONG_PTR>(mem_),
        //                 CC_FASTCALL,
        //                 str.return_type,
        //                 static_cast<UINT>(str.argument_values.size()),
        //                 str.argument_types.data(),
        //                 str.argument_values.data(),
        //                 &result);
        //
        //    return result;
        //}


       
        #else

        #endif

        size_t size() const noexcept
        {
            return size_;
        }

        bool is_valid() const noexcept
        {
            return mem_ != nullptr;
        }

        explicit operator bool() const noexcept
        {
            return is_valid();
        }


        
    };

}