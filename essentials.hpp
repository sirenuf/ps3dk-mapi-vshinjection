#pragma once

#include <sys/cdefs.h>         // CDECL*
#include <sys/lv2_syscall.h>   // lv2syscall1
#include <sys/prx.h>           // SYS_PRX_RESIDENT, SYS_PRX_STOP_OK
#include <sys/moduleexport.h>  // SYS_MODULE_*
#include <sys/ppu_thread.h>

#include <lv2/syscalls.h>

namespace Core {
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

    // The usermode sys_ppu_thread_exit call expects a TLS.
    // Mamba does not provide a TLS and therefore we crash.
    // Fix is using the raw syscall for Mamba threads, and
    // doing nothing else.
    static void RawExitThread(u64 ret_val)
    {
        constexpr u64 SYSCALL_PPU_THREAD_EXIT = 41;
        lv2syscall1(SYSCALL_PPU_THREAD_EXIT, ret_val);
    }

    static void SendPrxStopSignal(sys_prx_start_stop_module_option_t* options)
    {
        lv2syscall3(
            SYSCALL_PRX_STOP_MODULE,     // Syscall number 482 for stop module.
            sys_prx_get_my_module_id(),
            NULL,                        // Flags, no flags.
            (u64)options
        );
    }

    static void SendUnloadSuccessAndExitRaw()
    {
        // https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/lv2/sys_prx.cpp#L687
        // Build the call yourself so we can send the last remaining cmd = 2 part out.
        sys_prx_start_stop_module_option_t options;
            options.size = sizeof(options);
            options.cmd  = 2;        // Report.
            options.res  = CELL_OK;  // Set our state so MAPI can unload us.

        SendPrxStopSignal(&options);

        // After calling, exit the thread.
        RawExitThread(CELL_OK);
    }

    // Inline. This is reusable code but it cannot add a callstack or else we break LR before we have backed it up.
    static inline bool StartedByPayloadCheck()
    {
        // We need to store the return address from LR as the first thing we do.
        // Because the Cell/B.E. is running with 32 bit pointers, GCC's __builtin_return_address
        // will only return the low half, 32 bits, instead of full 64
        u64 retAddr;
        asm volatile (
            "mflr %0"         // Save LR in the beginning to a register of gcc's choosing.
            : "=r" (retAddr)  // Output: C-variables that recieve a result.
                              //  "=" is write only constraint. "r" is use general purpose registers in ppc.
                              // retAddr now contains LR's value.
        );

        // We need to check that the high 32 bits are the signature.
        //
        // When Mamba injects a prx, Lv2 will then create a new thread in the desired parent
        // and then immediately start the module's entry (mapped SYS_MODULE_START function) with a new thread
        // and give that thread a deliberate bad return address.
        // The signature is 0xBADADD00<thread id shifted 4 bits>
        //
        // So to circumvent a Lv2 exception, we can handle this safely.
        // Just check for the high BADADD00 signature on the LR we just saved in this function.

        // if retaddr is 0xbadadd00 10400e50, we can therefore shift right to only check for BADADD00.
        // retAddr >> 32 becomes 0x00000000 badadd00.
        return ((retAddr >> 32) == 0xBADADD00);
    }
}
