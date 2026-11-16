#pragma once
#include <ppu-types.h>

/**
 * This namespace contains fixes and workarounds for the addressed sub-namespace project
 */
namespace Core
{
    // Original is private. Official SDK and PS3DK are incomplete.
    // RPCS3 has it reverse engineered.
    // Kernel:
    //  rpcs3/Emu/Cell/lv2/
    //
    // Usermode:
    //  rpcs3/Emu/Cell/Modules/

    // The real type, as RPCS3 puts it:
    // https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/lv2/sys_prx.h#L107

    struct sys_prx_start_stop_module_option_t {
        u64 size;           // The size of this struct.
        u64 cmd;            // What mode to pick.
                            //      1 is Query   -  Ask lv2 about the stop_module address or epilogue.
                            //      2 is Report  -  Tell lv2 that the module has stopped or failed. Lv2 will then unload the module if stopped

        // Values to pass.
        // The prefix corresponds to what is used respective to the `cmd` set.
        // They are never used simultaneously and only tied to one `cmd`, lv2 will ignore the other properties.
        u64 entry;          // Query:   On response lv2 sets it to module_stop's address. -1 ignores.
        u64 res;            // Report:  Sets if module_stop returned successfully or not. 0 = success, lv2 will unload prx. 1 = error
        u64 entry2;         // Query:   On response lv2 sets it to module_epilogue's address if has one. -1 ignores.
    };
}