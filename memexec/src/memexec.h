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
    enum class type : std::uint8_t
    {
        // ------------------------- Special Cases ------------------------- //

        empty = 0,        //                         (void)
        void_ptr,         // ullVal | ulVal          (uintptr_t*),
        boolean,          // boolVal                 (short)

        // ------------------------ Signed Integers ------------------------ //

        i8,               // cVal                    (char)
        i16,              // iVal                    (short)
        i32,              // lVal | intVal           (long) | (int)
        i64,              // llVal                   (long long)

        // ----------------------- Unsigned Integers ----------------------- //

        u8,               // bVal                    (byte)
        u16,              // uiVal                   (unsigned short)
        u32,              // ulVal | uintVal         (unsigned long | int)
        u64,              // ullVal                  (unsigned long long)

        // ------------------------ Floating Points ------------------------ // 

        f32,              // fltVal                  (float)
        f64               // dblVal                  (double)

        // ----------------------------------------------------------------- //
    };
 
    struct value
    {
    public:
        
         type t;

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

        value() noexcept : t(type::empty), void_ptr(nullptr) {}

        value(void* val) noexcept : t(type::void_ptr), void_ptr(val) {}
        value(bool val) noexcept : t(type::boolean), boolean(val) {}

        value(std::int8_t val) noexcept : t(type::i8), i8(val) {}
        value(std::int16_t val) noexcept : t(type::i16), i16(val) {}
        value(std::int32_t val) noexcept : t(type::i32), i32(val) {}
        value(std::int64_t val) noexcept : t(type::i64), i64(val) {}

        value(std::uint8_t val) noexcept : t(type::u8), u8(val) {}
        value(std::uint16_t val) noexcept : t(type::u16), u16(val) {}
        value(std::uint32_t val) noexcept : t(type::u32), u32(val) {}
        value(std::uint64_t val) noexcept : t(type::u64), u64(val) {}

        value(float  val) noexcept : t(type::f32), f32(val) {}
        value(double val) noexcept : t(type::f64), f64(val) {}

        value(const value&) noexcept = default;
        value& operator=(const value&) noexcept = default;

        value(value&&) noexcept = default;
        value& operator=(value&&) noexcept = default;

        ~value() = default;
        
        static VARTYPE convert(type t = type::empty) noexcept
        {
            switch (t)
            {
                case type::empty:    return VT_EMPTY;
                case type::void_ptr: return VT_UINT_PTR;
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

                default:             return VT_EMPTY;
            }
        }

        static type convert(VARTYPE v = VT_EMPTY)
        {
            VARTYPE base_type = v & VT_TYPEMASK;

            switch (base_type)
            {
                case VT_EMPTY:    return type::empty;
                case VT_UINT_PTR: return type::void_ptr;
                case VT_BOOL:     return type::boolean;
                case VT_I1:       return type::i8;
                case VT_I2:       return type::i16;
                case VT_I4:       return type::i32;
                case VT_INT:      return type::i32;
                case VT_I8:       return type::i64;
                case VT_UI1:      return type::u8;
                case VT_UI2:      return type::u16;
                case VT_UI4:      return type::u32;
                case VT_UINT:     return type::u32;
                case VT_UI8:      return type::u64;
                case VT_R4:       return type::f32;
                case VT_R8:       return type::f64;

                default:          throw std::runtime_error("Unsupported \"VARTYPE\" type.");
            }
        }

        static bool convert(VARIANT_BOOL v_b) noexcept
        {
            return v_b != VARIANT_FALSE;
        }

        static VARIANT convert(value v) 
        {
            // Implement
        }

        static value convert(VARIANT v)
        {
            VARTYPE base_type = v.vt & VT_TYPEMASK;

            switch (base_type)
            {
                case VT_EMPTY:    return value();
#ifdef _WIN64
                case VT_UINT_PTR: return value(reinterpret_cast<void*>(v.ullVal));
#else
                case VT_UINT_PTR: return value(reinterpret_cast<void*>(v.ulVal));
#endif
                case VT_BOOL:     return value(convert(v.boolVal));
                case VT_I1:       return value(static_cast<std::int8_t>(v.cVal));
                case VT_I2:       return value(static_cast<std::int16_t>(v.iVal));
                case VT_I4:       return value(static_cast<std::int32_t>(v.lVal));
                case VT_INT:      return value(static_cast<std::int32_t>(v.intVal));
                case VT_I8:       return value(static_cast<std::int64_t>(v.llVal));
                case VT_UI1:      return value(static_cast<std::uint8_t>(v.bVal));
                case VT_UI2:      return value(static_cast<std::uint16_t>(v.uiVal));
                case VT_UI4:      return value(static_cast<std::uint32_t>(v.ulVal));
                case VT_UINT:     return value(static_cast<std::uint32_t>(v.uintVal));
                case VT_UI8:      return value(static_cast<std::uint64_t>(v.ullVal));
                case VT_R4:       return value(static_cast<float>(v.fltVal));
                case VT_R8:       return value(static_cast<double>(v.dblVal));

                default:          throw std::runtime_error("Unsupported \"VARIANT\" type.");
            }
        }
    };

    class rcg
    {
    private:
        
        void* mem_ = nullptr;
        std::size_t size_ = 0;

        void cleanup() noexcept
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
            type return_type { };
            std::vector<type> arguments_types { };
            std::vector<value*> arguments_values { }; // Implement
        };

        rcg() noexcept {}

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


#ifdef _WIN64

        value assemble_and_call(const function_structure& str)
        {
            VARTYPE return_type = value::convert(str.return_type);

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
        
            return value::convert(result);
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