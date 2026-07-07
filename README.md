<hr/>
<h1 align="center">Memexec</h1>
<p align="center">
  <strong>Small header only library writen in C++ with the intent of easy runtime machine code execution on windows.</strong>
</p>
<hr/>
<div align="center">
  <img src="https://img.shields.io/badge/C++26-24292F?logo=cplusplus&logoColor=red"/>
  <img src="https://img.shields.io/badge/Win32-24292F?"/>
</div>
</br>
<h2 align="center">Overview</h2>
<ul>
  <li>
    Memexec takes raw x86/x64 machine code bytes that you provide and simply executes them in place or, if you know the function's structure ahead of time, hands
    you back a real function pointer instead.
  </li>
  <li>
    On top of that, memexec provides a set of string conversion helpers for going back and forth between
    <code>format</code>/<code>callconv</code>/<code>type</code>/<code>value</code>/<code>machine code</code> and <code>std::string</code>.
    Useful if any of that information comes from outside your C++ source (a config file, a CLI flag, a UI) rather than being known at compile time.
  </li>
</ul>
</br>
<h2 align="center">Features</h2>
<p>
  <strong>Two execution engines with different tradeoffs</strong>
</p>
<ul>
  <li>
    First <strong>MCXE</strong> is the direct engine it either hands you a function pointer or executes in place, with no overhead beyond a
    <code>reinterpret_cast</code>.<br/>
    <strong>The tradeoff:</strong> you need to know the function's structure at compile time.
  </li>
  <li>
    Second <strong>RFIE</strong> is built around <code>DispCallFunc</code> from Win32's OLE Automation. It can execute functions without
    their structure being known until runtime, which is useful if you need to call arbitrary functions decided on the fly.<br/>
    <strong>The tradeoff:</strong> overhead.
  </li>
  <li>
    On x86, both engines can target a specific calling convention: MCXE via the <code>callconv</code> template argument on <code>get</code>/<code>call</code>.
    RFIE via <code>function_structure::call_conv</code> (defaults to <code>cc_stdcall</code> if left unset). On x64 there's just the one ABI, so both collapse to it
    regardless of what's asked for.
  </li>
</ul>
<h2 align="center">Getting started</h2>
<p>
  Copy <code>memexec.h</code> into your project and include it.
</p>

```cpp
#include "memexec.h"
```  
<p>
  That's the whole install, no build step and nothing beyond the Windows SDK to link against. The header auto-links <code>oleaut32.lib</code> on MSVC/clang-cl. If you   dont like this behavior or you are using diffrent compiler simply define <code>MEMEXEC_NO_AUTOLINK</code> macro before the include. 
</p>

```cpp
#define MEMEXEC_NO_AUTOLINK
#include "memexec.h"
```
<p>
  Note that the compiler needs to have at least C++23 support.
</p>

<h2 align="center">Usage</h2>
<h3>MCXE</h3>
<p>For this example, I'll call this square function extracted from a .cfg file that was compiled for a 32-bit architecture using the __stdcall calling convention.</p>

```cpp

// 8B 44 24 04     mov eax, [esp+4]
// 0F AF C0        imul eax, eax
// C2 04 00        ret 4

std::string_view cfg_format = "hex";
std::string_view cfg_code = "0x8B | 0x44 | 0x24 | 0x04 | 0x0F | 0xAF | 0xC0 | 0xC2 | 0x04 | 0x00";

auto format = memexec::string_to_format(cfg_format);

if (!format)
{
    return 0;
}

auto code = memexec::string_to_code(cfg_code, " | ", format.value());

if (!code)
{
    return 0;
}

memexec::mcxe engine;

if (engine.register_function(code.value()))
{
    auto result = engine.call<memexec::callconv::cc_stdcall, int, int>(2);

    if (result)
    {
        std::cout << result.value(); // -> 4
    }
}

```  

<h3>RFIE</h3>

```cpp
    #include "memexec.h"
    ...
```  

<h2 align="center">Safety</h2>
<p>
  This library runs arbitrary bytes on pages it marks executable, with no validation of what's actually in them.
</p>
<p>
  Don't feed it bytes you don't trust.
</p>
