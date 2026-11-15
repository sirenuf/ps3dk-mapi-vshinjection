cmake_minimum_required(VERSION 3.20)
project(hello_vsh CXX)

# Pulls in the PS3DK helpers (ps3_add_prx, ps3_add_self, ...).
# Found through CMAKE_MODULE_PATH, which the PS3DK toolchain file sets up.
include(ps3-self)

# Redundant for the compiler, but is a quick and easy way to make clangd find the right includes.
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# The module itself. It is an "executable" as far as CMake is concerned;
# ps3_add_prx below swaps in the PRX linker script/crt and turns the linked
# ELF into a .prx afterwards.
add_executable(hello_vsh
    export.cpp)

# Post-build: ELF -> hello_vsh.prx (prx-gen build + check), then SIGN produces
#   hello_vsh.sprx       real-signed (make_sprx)
#   hello_vsh.fake.sprx  fake-signed (fself)
# Module name/version come from SYS_MODULE_INFO in export.cpp.
ps3_add_prx(hello_vsh
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hello_vsh.prx"
    SIGN)

# Workaround for a PS3DK bug (still in v0.27): its lv2-prx.ld gives the module's
# segment the flags 0x7. The PS3 refuses a segment without SPU permission bits
# (sys_prx_load_module returns 0x80010007), so webMAN can't load the module.
# Link with a copy that adds the SPU-read bit (0x00400007), like Sony's modules.
file(READ "${PS3DK}/ppu/lib/lv2-prx.ld" _prx_ld)
string(REPLACE "FLAGS(0x01 | 0x02 | 0x04)" "FLAGS(0x00400007)" _prx_ld "${_prx_ld}")
if(NOT _prx_ld MATCHES "FLAGS\\(0x00400007\\)")
    message(FATAL_ERROR "PS3DK's lv2-prx.ld has changed: check if this workaround is still needed.")
endif()
file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/lv2-prx.ld" CONTENT "${_prx_ld}" @ONLY)
get_target_property(_link_options hello_vsh LINK_OPTIONS)
list(TRANSFORM _link_options REPLACE "^-Wl,-T,.*" "-Wl,-T,${CMAKE_BINARY_DIR}/lv2-prx.ld")
set_target_properties(hello_vsh PROPERTIES LINK_OPTIONS "${_link_options}")
set_property(TARGET hello_vsh APPEND PROPERTY LINK_DEPENDS "${CMAKE_BINARY_DIR}/lv2-prx.ld")
