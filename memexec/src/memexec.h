#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <oleauto.h>
#include <stdexcept>
#include <vector>
#include <span>
#include <array>
#include <string_view>

#pragma comment(lib, "oleaut32.lib")

class memexec
{
private:

    static constexpr inline std::array<std::string_view, 13> type_table_ = {"void", "void_ptr", "bool", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64"};
    static constexpr inline std::array<std::string_view, 3> callconv_table_ = {"stdcall", "ccdecl", "fastcall"};

public:

    enum class callconv : std::uint8_t
    {
        stdcall = 0,
        ccdecl,
        fastcall
    };

    static constexpr callconv string_to_callconv(std::string_view str) noexcept
    {
        for (std::uint8_t i = 0; i < callconv_table_.size(); i++)
        {
            if (callconv_table_[i] == str)
            {
                return static_cast<callconv>(i);
            }
        }
    }

    static constexpr std::string_view callconv_to_string(callconv call) noexcept
    {
        return callconv_table_[static_cast<std::uint8_t>(call)];
    }

    
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
    
    static constexpr type string_to_type(std::string_view str) noexcept
    {
        for (std::uint8_t i = 0; i < type_table_.size(); i++)
        {
            if (type_table_[i] == str)
            {
                return static_cast<type>(i);
            }
        }
    }

    static constexpr std::string_view type_to_string(type t) noexcept
    {
        return type_table_[static_cast<std::uint8_t>(t)];
    }

 
    class value
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
    };

    /* Memory Stager */
    class memstager
    {
    protected:
        
        void* mem_ = nullptr;
        std::size_t size_ = 0;

        void virtual cleanup() noexcept
        {
            if (mem_)
            {
                VirtualFree(mem_, 0, MEM_RELEASE);
                mem_ = nullptr;
                size_ = 0;
            }
        }

    public:
        memstager() noexcept = default;

        memstager(const std::uint8_t* code, std::size_t size)
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

        memstager(std::span<std::uint8_t> code) : memstager(code.data(), code.size()) {}

        memstager(const memstager&) = delete;
        memstager& operator=(const memstager&) = delete;

        memstager(memstager&& other) noexcept : mem_(other.mem_), size_(other.size_)
        {
            other.mem_ = nullptr;
            other.size_ = 0;
        }

        memstager& operator=(memstager&& other) noexcept
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

        virtual ~memstager()
        {
            cleanup();
        }

        std::size_t size() const noexcept
        {
            return size_;
        }

        virtual bool is_executable() const noexcept = 0;

        explicit operator bool() const noexcept
        {
            return is_executable();
        }

    };

    /* Machine Code Execution Engine */
    class mcxe : public memstager
    {
    public:

        mcxe() noexcept : memstager() {}

        mcxe(const std::uint8_t* code, std::size_t size) : memstager(code, size) { }

        mcxe(std::span<std::uint8_t> code) : memstager(code) {}

        mcxe(const mcxe&) = delete;
        mcxe& operator=(const mcxe&) = delete;

        mcxe(mcxe&& other) noexcept : memstager(std::move(other)) {}

        mcxe& operator=(mcxe&& other) noexcept
        {
            memstager::operator=(std::move(other));
            return *this;
        }

        ~mcxe() override = default;


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
        inline auto get() const noexcept -> ReturnType(*)(ParamTypes...)
        {
            return reinterpret_cast<ReturnType(*)(ParamTypes...)>(mem_);
        }

        template <typename ReturnType, typename... Params>
        inline ReturnType call(Params&&... args) const noexcept
        {
            return get<ReturnType, Params...>()(std::forward<Params>(args)...);
        }

        bool is_executable() const noexcept override
        {
            return mem_ != nullptr;
        }
    };

    /* Runtime Function Invocation Engine */
    class rfie : public memstager
    {
    
    public:

        struct function_structure
        {
            callconv call_conv = callconv::stdcall;
            type return_type = type::empty;
            std::vector<type> arguments_types{ };
            std::vector<value> arguments_values{ };
        };

    private:

        struct dispcallfunc_structure
        { 
            CALLCONV call_conv = CC_STDCALL;
            VARTYPE return_type = VT_EMPTY;
            std::vector<VARTYPE> arguments_types { };
            std::vector<VARIANT> arguments_values { };
            std::vector<VARIANTARG*> arguments_value_ptrs { };
        };

        dispcallfunc_structure str_ { };


        void cache_dispcallfunc_structure(const function_structure& str)
        {
            str_.call_conv = convert(str.call_conv);
            str_.return_type = convert(str.return_type);

            str_.arguments_types.reserve(str.arguments_types.size());
            str_.arguments_values.reserve(str.arguments_values.size());
            str_.arguments_value_ptrs.reserve(str.arguments_values.size());

            for (size_t i = 0, end = str.arguments_values.size(); i < end; i++)
            {
                size_t  reversed_index = end - 1 - i;

                str_.arguments_types.emplace_back(convert(str.arguments_types[reversed_index]));
                str_.arguments_values.emplace_back(convert(str.arguments_values[reversed_index]));
                str_.arguments_value_ptrs.emplace_back(&str_.arguments_values.back());
            }
        }


        constexpr CALLCONV convert(callconv call) noexcept
        {
            #ifdef _WIN64

                return CC_STDCALL;

            #else

                switch(call)
                {
                    case callconv::stdcall:  return CC_STDCALL;
                    case callconv::ccdecl:   return CC_CDECL;
                    case callconv::fastcall: return CC_FASTCALL;

                    default:                 throw std::runtime_error("Unsupported \"memexec::callconv\"");
                }

            #endif
        }

        static VARTYPE convert(type t = type::empty)
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

                default:             throw std::runtime_error("Unsupported \"memexec::type\"");
            }
        }

        static bool convert(VARIANT_BOOL var_bool) noexcept
        {
            return var_bool != VARIANT_FALSE;
        }

        static VARIANT_BOOL convert(bool b) noexcept
        {
            return (b) ? VARIANT_TRUE : VARIANT_FALSE;
        }

        static VARIANT convert(value val)
        {
            VARIANT var;
            VariantInit(&var);

            switch (val.t)
            {
                    case type::empty:
                    {
                        var.vt = VT_EMPTY;
                    }
                    break;

                #ifdef _WIN64

                    case type::void_ptr:
                    {
                        var.vt = VT_UINT_PTR;
                        var.ullVal = reinterpret_cast<ULONGLONG>(val.void_ptr);
                    }
                    break;

                #else

                    case type::void_ptr:
                    {
                        var.vt = VT_UINT_PTR;
                        var.ulVal = reinterpret_cast<ULONG>(val.void_ptr);
                    }
                    break;

                #endif

                case type::boolean:
                {
                    var.vt = VT_BOOL;
                    var.boolVal = convert(val.boolean);
                }
                break;

                case type::i8:
                {
                    var.vt = VT_I1;
                    var.cVal = static_cast<CHAR>(val.i8);
                }
                break;

                case type::i16:
                {
                    var.vt = VT_I2;
                    var.iVal = static_cast<SHORT>(val.i16);
                }
                break;

                case type::i32:
                {
                    var.vt = VT_I4;
                    var.lVal = static_cast<LONG>(val.i32);
                }
                break;

                case type::i64:
                {
                    var.vt = VT_I8;
                    var.llVal = static_cast<LONGLONG>(val.i64);
                }
                break;

                case type::u8:
                {
                    var.vt = VT_UI1;
                    var.bVal = static_cast<BYTE>(val.u8);
                }
                break;

                case type::u16:
                {
                    var.vt = VT_UI2;
                    var.uiVal = static_cast<USHORT>(val.u16);
                }
                break;

                case type::u32:
                {
                    var.vt = VT_UI4;
                    var.ulVal = static_cast<ULONG>(val.u32);
                }
                break;

                case type::u64:
                {
                    var.vt = VT_UI8;
                    var.ullVal = static_cast<ULONGLONG>(val.u64);
                }
                break;

                case type::f32:
                {
                    var.vt = VT_R4;
                    var.fltVal = static_cast<FLOAT>(val.f32);
                }
                break;

                case type::f64:
                {
                    var.vt = VT_R8;
                    var.dblVal = static_cast<DOUBLE>(val.f64);
                }
                break;

                default:
                {
                    throw std::runtime_error("Unsupported \"memexec::type\"");
                }
            }

            return var;
        }

        static value convert(VARIANT var)
        {
            VARTYPE base_type = var.vt & VT_TYPEMASK;

            switch (base_type)
            {
                case VT_EMPTY:    return value();

                #ifdef _WIN64

                    case VT_UINT_PTR: return value(reinterpret_cast<void*>(var.ullVal));

                #else

                    case VT_UINT_PTR: return value(reinterpret_cast<void*>(var.ulVal));

                #endif

                case VT_BOOL:     return value(convert(var.boolVal));
                case VT_I1:       return value(static_cast<std::int8_t>(var.cVal));
                case VT_I2:       return value(static_cast<std::int16_t>(var.iVal));
                case VT_I4:       return value(static_cast<std::int32_t>(var.lVal));
                case VT_INT:      return value(static_cast<std::int32_t>(var.intVal));
                case VT_I8:       return value(static_cast<std::int64_t>(var.llVal));
                case VT_UI1:      return value(static_cast<std::uint8_t>(var.bVal));
                case VT_UI2:      return value(static_cast<std::uint16_t>(var.uiVal));
                case VT_UI4:      return value(static_cast<std::uint32_t>(var.ulVal));
                case VT_UINT:     return value(static_cast<std::uint32_t>(var.uintVal));
                case VT_UI8:      return value(static_cast<std::uint64_t>(var.ullVal));
                case VT_R4:       return value(static_cast<float>(var.fltVal));
                case VT_R8:       return value(static_cast<double>(var.dblVal));

                default:          throw std::runtime_error("Unsuported \"VARTYPE\"");
            }
        } 
        

        void cleanup() noexcept override
        {
            memstager::cleanup();

            str_.call_conv = CC_STDCALL;
            str_.return_type = VT_EMPTY;

            str_.arguments_types.clear();
            str_.arguments_values.clear();
            str_.arguments_value_ptrs.clear();
            
            str_.arguments_types.shrink_to_fit();
            str_.arguments_values.shrink_to_fit();
            str_.arguments_value_ptrs.shrink_to_fit(); 
        }

    public: 
        
        rfie() noexcept : memstager() {}

        rfie(const std::uint8_t* code, std::size_t size, const function_structure& str) : memstager(code, size) 
        { 
            try
            {
                cache_dispcallfunc_structure(str);
            }
            catch (...)
            {
                cleanup();
                throw;
            }
        }

        rfie(std::span<std::uint8_t> code, const function_structure& str) : rfie(code.data(), code.size(), str) {}

        rfie(const rfie&) = delete;
        rfie& operator=(const rfie&) = delete;

       rfie(rfie&& other) noexcept : memstager(std::move(other))
       {
           str_.call_conv = other.str_.call_conv;
           str_.return_type = other.str_.return_type;
           str_.arguments_types = std::move(other.str_.arguments_types);
           str_.arguments_values = std::move(other.str_.arguments_values);
           str_.arguments_value_ptrs = std::move(other.str_.arguments_value_ptrs);
           
           other.str_.call_conv = CC_STDCALL;
           other.str_.return_type = VT_EMPTY;
       }
       
       rfie& operator=(rfie&& other) noexcept
       {
           if (this != &other)
           {
               memstager::operator=(std::move(other));

               str_.call_conv = other.str_.call_conv;
               str_.return_type = other.str_.return_type;
               str_.arguments_types = std::move(other.str_.arguments_types);
               str_.arguments_values = std::move(other.str_.arguments_values);
               str_.arguments_value_ptrs = std::move(other.str_.arguments_value_ptrs);

               other.str_.call_conv = CC_STDCALL;
               other.str_.return_type = VT_EMPTY;
           }

           return *this;
       }

       ~rfie() override = default;
       

        bool register_function(const std::uint8_t* code, std::size_t size, const function_structure& str) noexcept
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
                memstager::cleanup();
                return false;
            }

            try
            {
                cache_dispcallfunc_structure(str);
            }
            catch(...)
            {
                cleanup();
                return false;
            }

            return true;
        }

        bool register_function(std::span<std::uint8_t> code, const function_structure& str) noexcept
        {
            return register_function(code.data(), code.size(), str);
        }


        value call()
        { 
            VARIANT result;
            VariantInit(&result);

            if(FAILED(DispCallFunc(nullptr, 
                                   reinterpret_cast<ULONG_PTR>(mem_),
                                   str_.call_conv,
                                   str_.return_type,
                                   static_cast<UINT>(str_.arguments_values.size()),
                                   str_.arguments_types.data(),
                                   str_.arguments_value_ptrs.data(),
                                   &result)))
            {  
                throw std::runtime_error("DispCallFunc failed.");
            }
        
            return convert(result);
        } 
        
        bool is_executable() const noexcept override
        {
            return mem_
                && !str_.arguments_types.empty()
                && !str_.arguments_values.empty()
                && !str_.arguments_value_ptrs.empty();
        }
    };
};