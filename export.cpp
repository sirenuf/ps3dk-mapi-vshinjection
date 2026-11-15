#include "essentials.hpp"

SYS_MODULE_INFO(ExampleModule, 0, 1, 0);
SYS_MODULE_START(prx_entry_wrapper);
SYS_MODULE_STOP(prx_stop_wrapper);

CDECL_BEGIN
int prx_entry_wrapper(size_t args, void *argp)
{
    bool payload = Core::StartedByPayloadCheck();

    if (payload) {
        // Create a new thread that runs the actual main function.
        
        // Tell lv2 to destroy the thread since we are ran anonymously with a bad return address.
        // skips the exception.
        Core::RawExitThread(CELL_OK);
    }

    // The module is loaded normally, not by a payload. Exit normally.
    return SYS_PRX_RESIDENT;
}

int prx_stop_wrapper(size_t args, void *argp)
{
    bool payload = Core::StartedByPayloadCheck();

    if (payload) {
        // Again, unlike the library sys_prx_* functions, MAPI unloads modules differently.
        // 
        // https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/Modules/sys_prx_.cpp#L123
        // RPCS3 has the usermode and kernel call reverse engineered. We can peep in here to see
        // what should happen.
        //
        // Looking at the usermode sys_prx_stop_module function, it firsts queries the kernel with
        // cmd = 1 to get the entry address, aka module_stop.
        // if the res is successful, it then executes module_stop via `entryx`.
        //
        // It then does does the same syscall again but with `cmd` put to `2` and the result from the
        // entryx.
        // If the result was 0, the kernel will then proceed to actually unload the module.
        //
        // This is what we're missing when MAPI/MAMBA unloads us. It only does the first cmd = 1 step
        // and hands the rest to us. Therefore, we have to finish the last state ourselves.
        // SendUnloadSuccessAndExit() will build the final step and also exit our thread, since we also
        // suffer from the module_start issue above.
        Core::SendUnloadSuccessAndExitRaw();
    }

    // Return like normal if we're not startedByPayload
    return SYS_PRX_STOP_OK;
}
CDECL_END