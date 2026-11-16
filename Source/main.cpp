#include "Core/Cobra/Cobra.hpp"
#include "Core/PS3DK/PS3DK.hpp"

#include <sys/prx.h>
#include <sys/moduleexport.h>
#include <sys/ppu_thread.h>

// No heap, does not work with PRXs on PS3DK yet
// #include <iostream>
// Because of the segment situation on the PRX linker in PS3DK this is not possible either.
// #include <cstdio>

SYS_MODULE_INFO(ExampleModule, 0, 69, 42);
SYS_MODULE_START(prx_entry_wrapper);
SYS_MODULE_STOP(prx_stop_wrapper);

void PluginMain(void* arg)
{
    Core::PS3DK::printf("Hello World\n");

    // Do not return here either. Lv2 will always give us a 0xBADADD00 LR.
    // We can now use the usermode library though instead of a raw syscall
    // since this thread now has TLS.

    // Use appropriate return codes
    sysThreadExit(SYS_PRX_STOP_OK);
}

void PluginMainShutdown(void* arg)
{
    Core::PS3DK::printf("Stopping\n");

    // Make sure you stop any other threads here and wait for them and cleanup.

    // Use appropiate return codes
    sysThreadExit(SYS_PRX_STOP_OK);
}

CDECL_BEGIN
int prx_entry_wrapper(size_t args, void *argp)
{
    bool payload = Core::Cobra::StartedByPayloadCheck();

    // Create a new healthy main thread which has a TLS thanks to the usermode library.
    //
    // Do not use sys_ppu_thread_create. PS3DK routes it through librt which is not
    // supported in PRXs. PSL1GHT's sysThreadCreate does the same thing, but uses the
    // raw liblv2 usermode function sys_ppu_thread_create and not PS3DK's wrapper.
    u64 returnCode;
    sys_ppu_thread_t mainThreadSafe;
    sysThreadCreate(&mainThreadSafe, PluginMain, nullptr, 1100, 8192, SYS_PPU_THREAD_CREATE_JOINABLE, (char*)"Plugin main thread");

    sysThreadJoin(mainThreadSafe, &returnCode);

    if (payload) {
        // Exit raw because we have no TLS.
        Core::Cobra::RawExitThread(returnCode);
    }

    // The module is loaded normally, not by a payload. Exit normally.
    return returnCode;
}

int prx_stop_wrapper(size_t args, void *argp)
{
    bool payload = Core::Cobra::StartedByPayloadCheck();

    u64 shutdownReturnCode;
    sys_ppu_thread_t shutdownThreadId;
    sysThreadCreate(&shutdownThreadId, PluginMainShutdown, nullptr, 1100, 8192, SYS_PPU_THREAD_CREATE_JOINABLE, (char*)"Plugin shutdown thread");

    // Halt execution until the shutdown thread finishes. After that, exit with its return code.
    // If we don't wait for it, we risk unloading before finishing the thread which crashes it.
    sysThreadJoin(shutdownThreadId, &shutdownReturnCode);

    if (payload) {
        Core::Cobra::SendUnloadStatusAndExitRaw(shutdownReturnCode);
    }

    // Return like normal if we're not startedByPayload
    return shutdownReturnCode;
}
CDECL_END