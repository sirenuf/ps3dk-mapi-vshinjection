#pragma once
#include "../Core.hpp"

// typedefs
#include <ppu-types.h>

/*
 * This namespace contains fixes and workarounds for Cobra quirks and documents them.
 */
namespace Core::Cobra
{
    // Exit thread using the raw Lv2 syscall.
    // Cobra never gives the initial thread a TLS, which the
    // usermode sys_ppu_thread_exit expects.
    // So we crash. This only calls exit thread directly to Lv2,
    // and never touches the non-existant TLS.
    //
    // This is only reserved for module_start and module_stop
    // other healthy threads should always use the usermode wrappers.
    void RawExitThread(u64 ret_val);


    /**
    * @brief Sends a custom form to syscall 482 `SYSCALL_PRX_STOP_MODULE`.
    *
    * @param options The form.
    */
    void SendPrxStopSignal(sys_prx_start_stop_module_option_t* options);

    /**
     * @brief Builds a form and sends a `"module stop succeeded"` signal to Lv2 so it can then unload us. Then exits the thread with `RawExitThread()`.
     *
     * When unloading a module with MAPI's `ps3mapi_unload_process_modules()`, MAPI will first run `module_stop`
     * and then unload simultaneously.
     * It relies on Cobra's `prx_stop_module_with_thread()` wrapper which, unlike the `prx_start` counterpart, only sends the initial
     * `cmd = 1` to Lv2's `SYSCALL_PRX_STOP_MODULE` (number 482) syscall, aka `_sys_prx_stop_module()`.
     *
     * Usermode `liblv2` libraries like `sys_prx_stop_module()` does what Cobra should've.
     *  1. Send a `SYSCALL_PRX_STOP_MODULE` with `cmd = 1` (query) to get the address of `module_stop`.
     *   -> Lv2 sets PRX state to `PRX_STATE_STOPPING`.
     *
     *  2. Run `module_stop` and store its return value.
     *  3. Send a `SYSCALL_PRX_STOP_MODULE` with `cmd = 2` (report) and `res` set to the previous return value.
     *   -> Lv2 sets `PRX_STATE_STOPPED`
     * 
     * Cobra only does step 1 and 2 and returns back to the MAPI function.
     * So when MAPI then finally tries to run lv2's internal `prx_unload_module()` on the PRX, it fails
     * because Lv2 expects the PRX state to be `PRX_STATE_STOPPED`.
     *
     * We fix this in this function by building and sending the `cmd = 2` request ourselves. Lv2 happily unloads us then.
     *
     * @see MAMBA: [`ps3mapi_unload_process_modules()`](https://github.com/aldostools/Mamba/blob/master/stage2/ps3mapi_core.c#L502)
     * @see MAMBA: [`prx_stop_module_with_thread()`](https://github.com/aldostools/Mamba/blob/master/lv2/src/modules_c.c#L77)
     * @see RPCS3 liblv2: [`sys_prx_stop_module`](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/Modules/sys_prx_.cpp#L123)
     * @see RPCS3 lv2: [`_sys_prx_unload_module`](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/lv2/sys_prx.cpp#L746)
     */
    void SendUnloadStatusAndExitRaw(u64 returnCode);

    /**
     * @brief Adds a check that determines if we're started by MAMBA/COBRA/MAPI by looking at the LR. Needs to be at the beginning of module_start/stop.
     * 
     * Cobra doesn't wrap remotely injected modules `module_start()` call in a thread so it can return safely. So Lv2 gives the thread a poisoned `0xBADADD00` LR.
     * Returning makes lv2 complain. This function helps us safely handle it by checking for the poison LR signature. We can then exit the thread instead of returning.
     */
    [[gnu::always_inline]] inline bool StartedByPayloadCheck();
}

// Inlines
namespace Core::Cobra
{
    // Inline. This is reusable code but it cannot add a callstack or else we break LR before we have backed it up.
    // always_inline attribute to force the original historic behaviour of `inline`
    [[gnu::always_inline]] inline bool StartedByPayloadCheck()
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
        // When Cobra injects a prx, Lv2 will then create a new thread in the desired parent
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
