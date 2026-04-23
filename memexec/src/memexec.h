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
    namespace variant
    {     
        enum class type : std::uint8_t
        {
            // ------------------------- Special Cases ------------------------- //

            empty = 0,        // VT_EMPTY        (void)
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
 
        static VARTYPE convert(type t = type::empty)
        {
            switch (t)
            {
                case type::empty:    return VT_EMPTY;
                case type::void_ptr: return VT_PTR;
                case type::boolean:  return VT_BOOL;
                case type::i8:       return VT_I1;
                case type::i16:      return VT_I2;
                case type::i32:      return VT_I4;
                case type::i64:      return VT_I8;
                case type::u8:       return VT_UI1;
                case type::u16:      return VT_UI2;
                case type::u32:      return VT_UI4;
                case type::u64:      return VT_UI8;
                case type::f32:      return VT_R4;
                case type::f64:      return VT_R8;
            }
        }

        static type convert(VARTYPE v = VT_EMPTY)
        {
            // Implement
        }

        struct value
        {
        private:

            type t = type::empty;

        public:

            union
            {
                void* void_ptr;
                bool  boolean;

                std::int8_t  i8;
                std::int16_t i16;
                std::int32_t i32;
                std::int64_t i64;

                std::uint8_t  u8;
                std::uint16_t u16;
                std::uint32_t u32;
                std::uint64_t u64;

                float  f32;
                double f64;
            };

            value() : t(type::empty), void_ptr(nullptr) {}

            value(void* val) : t(type::void_ptr), void_ptr(val) {}
            value(bool  val) : t(type::boolean), boolean(val) {}

            value(std::int8_t  val) : t(type::i8), i8(val) {}
            value(std::int16_t val) : t(type::i16), i16(val) {}
            value(std::int32_t val) : t(type::i32), i32(val) {}
            value(std::int64_t val) : t(type::i64), i64(val) {}

            value(std::uint8_t  val) : t(type::u8), u8(val) {}
            value(std::uint16_t val) : t(type::u16), u16(val) {}
            value(std::uint32_t val) : t(type::u32), u32(val) {}
            value(std::uint64_t val) : t(type::u64), u64(val) {}

            value(float  val) : t(type::f32), f32(val) {}
            value(double val) : t(type::f64), f64(val) {}

            value(const value&) = default;
            value& operator=(const value&) = default;

            value(value&&) = default;
            value& operator=(value&&) = default;

            ~value() = default;

            bool is_void() const noexcept { return t == type::empty; }
        };

        static VARIANT convert(value v)
        {
            // Implement
        }
        
        static value convert(VARIANT v)
        {
            // Implement
        }
    };

    class rcg
    {
    private:
        
        void* mem_ = nullptr;
        std::size_t size_ = 0;
        
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
            variant::type return_type { };
            std::vector<variant::type> arguments_types { };
            std::vector<variant::value*> arguments_values { }; // Implement
        };

        rcg() {}

        rcg(const std::uint8_t* code, std::size_t size)
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


        bool register_function(const std::uint8_t* code, std::size_t size) noexcept
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

        variant::value assemble_and_call(const function_structure& str)
        {
            VARTYPE return_type = variant::convert(str.return_type);

            VARTYPE* arguments_types = ; // Implement

            VARIANTARG** arguments_values = ; // Implement
        
            VARIANT result;
            DispCallFunc(nullptr, 
                         reinterpret_cast<ULONG_PTR>(mem_),
                         CC_FASTCALL,
                         return_type,
                         static_cast<UINT>(str.arguments_values.size()),
                         arguments_types,
                         arguments_values,
                         &result);
        
            return variant::convert(result);
        }
       
        #else

        #endif

        std::size_t size() const noexcept
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