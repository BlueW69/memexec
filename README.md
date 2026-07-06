<hr/>
<h1 align="center">Memexec</h1>
<p align="center">
  <strong>Small header only library writen in C++ with the intent of easy runtime machine code execution on windows.</strong>
</p>
<hr/>
<div align="center">
	@@ -11,17 +11,76 @@
</br>
<h2 align="center">Overview</h2>
<ul>
  <li>
    Memexec takes raw x86/x64 machine code bytes that you provide and simply executes them in place or, if you know the function's structure ahead of time, hands
    you back a real function pointer instead.
  </li>
  <li>
    On top of that, memexec provides a set of string conversion helpers for going between format/callconv/type/value/machine code and plain text.
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
    First <strong>mcxe</strong> is the direct engine it either hands you a function pointer or executes in place, with no overhead beyond a
    <code>reinterpret_cast</code>.<br/>
    <strong>The tradeoff:</strong> you need to know the function's structure at compile time.
  </li>
  <li>
    Second <strong>rfie</strong> is built around <code>DispCallFunc</code> from Win32's OLE Automation. It can execute functions without
    their structure being known until runtime, which is useful if you need to call arbitrary functions decided on the fly.<br/>
    <strong>The tradeoff:</strong> overhead.
  </li>
  <li>
    On x86, both engines can target a specific calling convention: mcxe via the <code>callconv</code> template argument on <code>get</code>/<code>call</code>.
    rfie via <code>function_structure::call_conv</code> (defaults to <code>cc_stdcall</code> if left unset). On x64 there's just the one ABI, so both collapse to it
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

That's the whole install, no build step and nothing beyond the Windows SDK to link against. The header auto-links <code>oleaut32.lib</code> on MSVC/clang-cl. If you dont like this behavior or you are using diffrent compiler simply define <code>MEMEXEC_NO_AUTOLINK</code> macro before the include. 

```cpp
    #define MEMEXEC_NO_AUTOLINK
    #include "memexec.h"
```

Note that the compiler needs to have at least C++23 support.
</p>

<h2 align="center">Usage</h2>

<h3>mcxe</h3>

```cpp
    #include "memexec.h"
    ...
```  

<h3>rfie</h3>

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
