#include "Cobra.hpp"

#include <sys/ppu_thread.h>
#include <lv2/syscalls.h>
#include <sys/prx.h>

namespace Core::Cobra
{
    // The usermode sys_ppu_thread_exit call expects a TLS.
    // Cobra does not provide a TLS and therefore we crash.
    // Fix is using the raw syscall for Cobra threads, and
    // doing nothing else. Nothing leaks because there is
    // no tls anyways.
    void RawExitThread(u64 ret_val)
    {
        constexpr u64 SYSCALL_PPU_THREAD_EXIT = 41;
        lv2syscall1(SYSCALL_PPU_THREAD_EXIT, ret_val);
    }

    void SendPrxStopSignal(sys_prx_start_stop_module_option_t* options)
    {
        lv2syscall3(
            SYSCALL_PRX_STOP_MODULE,     // Syscall number 482 for stop module.
            sys_prx_get_my_module_id(),
            0,                           // Flags, no flags.
            (u64)options
        );
    }

    void SendUnloadStatusAndExitRaw(u64 returnCode)
    {
        // https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/lv2/sys_prx.cpp#L687
        // Build the call yourself so we can send the last remaining cmd = 2 part out.
        sys_prx_start_stop_module_option_t options;
            options.size = sizeof(options);
            options.cmd  = 2;           // Report.
            options.res  = returnCode;  // Set our response. 0 will mean MAPI unloads us.

        SendPrxStopSignal(&options);

        // After calling, exit the thread.
        RawExitThread(returnCode);
    }
}