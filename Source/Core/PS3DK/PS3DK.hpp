#pragma once
#include <ppu-types.h>

/*
 * Workarounds for PS3DK. It does not define _sys_printf but includes it in the stubs.
 * Heap allocation is also broken.
 */

extern "C" void _sys_printf(...);

namespace Core::PS3DK
{
    // Is in the PS3DK linked stubs but never declared anywhere.
    // Works, comes from lv2 itself I think (not glibc).
    void printf(...) asm("_sys_printf"); // Alias printf to _sys_printf
}