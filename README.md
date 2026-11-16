# Sample SPRX vsh plugin using PS3DK
This is a sample project which uses [PS3DK](https://github.com/FirebirdTA01/PS3DK) and compiles and runs on real CFW hardware.

**PS3DK** is in very early development and not ready yet but I think it is a cool example of what it can do.

With that said, the homebrew VSH plugin scene is really poorly documented and current payloads like COBRA and MAMBA with PS3MAPI handle remote SPRX injection very poorly.  
All of that is well documented in my code, look in the header files in [Source/Core/Cobra/Cobra.hpp](/Source/Core/Cobra/Cobra.hpp)

# Building
Make sure that at least `PS3DK` and `PS3_TOOLCHAIN_ROOT` are defined.

```bash
cmake --preset ps3
cmake --build build
```

Output is inside `build/` as `hello_vsh.sprx` and `hello_vsh.fake.sprx`. Both run and mount fine.
