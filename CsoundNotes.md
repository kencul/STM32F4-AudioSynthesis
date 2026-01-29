# Running Csound on STM32F407G Disc

## Edit crosscompile.cmake file in Daisy folder in Csound repo

STM32F407G uses a cortex-m4 chip, so the library must be built for it:

```
set(MCU "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")
```

Some trimming to flags that I'm not sure help or not:

```
set(OBJECT_GEN_FLAGS "${MCU} -fno-builtin -fno-exceptions -Wall -ffunction-sections -fdata-sections -fomit-frame-pointer -finline-functions")
```

## Changes to Custom.cmake in Daisy folder

Instead of downloading and installing the Daisy toolchain, I used the compiler and make alternative, Ninja, that STMCube comes with.

The arm-none-eabi behavior is identical, but Ninja complains about "multiple rules":

```
ninja: error: build.ninja:5972: multiple rules generate `libcsound.a`
```

This conflict happens because the script tries to handle "Bare Metal" as a special case, but it doesn't correctly "turn off" the standard library creation logic that follows it.

From `CmakeLists.txt`:

```
if(NOT IOS OR OSXCROSS_TARGET)
    if(NOT BARE_METAL)
        add_library(${CSOUNDLIB} SHARED ${libcsound_SRCS})
    else()
        # This is where the STM32 library is defined
        add_library(${CSOUNDLIB} STATIC ${libcsound_SRCS}) 
    endif()
    # ... additional properties ...
endif()
```

This states to make `libcsound.a`, compile all these source files.

Then later in `CmakeLists.txt`:

```
if(BUILD_STATIC_LIBRARY)
    # This creates a second, identical rule for the same file
    add_library(${CSOUNDLIB_STATIC} STATIC ${libcsound_static_SRCS}) 
    
    # ... 
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES OUTPUT_NAME ${CSOUNDLIB})
    # ...
endif()
```

Ninja seems to refuse to build instead of risking a non-deterministic build with two source files and compiler flags.

The easy solution to this was to set `BUILD_STATIC_LIBRARY` to `OFF` in Custom.cmake:

```
set(BUILD_STATIC_LIBRARY OFF)
```

With this, Ninja sees one rule for `libcsound.a`, so it successfully builds.

### Building

As I am on Windows, here were the powershell commands I used to build:

```powershell
mkdir build

cd build

cmake .. `
 -G "Ninja" `
 -DCMAKE_TOOLCHAIN_FILE="../Daisy/crosscompile.cmake" `
 -DCUSTOM_CMAKE="../Daisy/Custom.cmake"

ninja
```

## Manually Installing Library

As the make install command is for Daisy, I had to manually install the library files into my STMCube project.

I made a directory `Csound/` with subdirectories `lib/` and `include/`.

`libcsound.a` goes into `lib/`.

All files from `csound/include/` go into `Csound/include`, along with `version.h` and `float-version.h` from `csound/build/include/`.

The `CmakeLists.txt` must be pointed towards these new files:

```
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # ... my existing paths ...
    Csound/include
)
```
```
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # ... my existing libs ...
    ${CMAKE_CURRENT_SOURCE_DIR}/Csound/lib/libcsound.a
)
```

## Testing Csound

To test if the library works, I added this to my main function:

```cpp
CSOUND* cs = csoundCreate(NULL, NULL);
if(cs != NULL) {
    // If it compiles and turns on the orange LED, the library works
    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
    csoundDestroy(cs);
}
```

Building this code results in a two errors:

1. ```region 'FLASH' overflowed by 1207772 bytes FLASH: 2256348 B 1 MB 215.18%```

The STM32F407G board only has 1MB of flash, and the program takes over 2MB.

2. ```undefined reference to _gettimeofday```

The program reaches for a OS function called `_gettimeofday` to stay in time. The board doesn't have such function.

## _gettimeofday

Csound needs this function to work, so I made a replacement function based on HAL functions.

`App/Src/csound_stub.cpp`
```cpp
#include <sys/time.h>
#include "stm32f4xx_hal.h"

extern "C" {
    int _gettimeofday(struct timeval *tv, void *tzvp) {
        if (tv) {
            uint32_t ticks = HAL_GetTick(); 
            tv->tv_sec = ticks / 1000;
            tv->tv_usec = (ticks % 1000) * 1000;
        }
        return 0;
    }
}
```

I added this file to CmakeLists.txt, and reordered the libcsound library file to load before standard C libraries.

```
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/Inc
    App/Inc
    Middlewares/Csound/include
)
```
```
# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Csound/lib/libcsound.a
    stdc++
    m
    nosys
)
```

This removed the undefined function error.

## Size Issue

The program is currently around 2.2MB out of the 1MB of flash my board has. I must aggressively prune Csound to fit on the board.

To have a starting point, not the build output states the program uses 2.25MB of flash, and the library file is at 4.1MB.

First, I changed the Custom.cmake file to optimize further for space:

```
set(CMAKE_BUILD_TYPE "MinSizeRel") # Changed from Release to MinSizeRel (-Os)

# ...

# --- Diabling more unneeded features ---
set(BUILD_PLUGINS OFF)           # Forces all opcodes to be internal/static
set(BUILD_DSSI_OPCODES OFF)      # Removes legacy synth interface
set(BUILD_OSC_OPCODES OFF)       # Removes Networking/Open Sound Control
set(BUILD_DOCS OFF)              # No doxygen
set(BUILD_TESTS OFF)             # No unit tests
set(USE_CURL OFF)                # Removes web-loading code

# --- Flag Optimizations ---
# -Os is "Optimize for Size". -ffunction-sections and -fdata-sections allows the linker to delete code that your specific app doesn't call.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections -fno-rtti")

# ...
```

This reduced the library file size to 3.5MB. Building with this file results in 1.75MB on flash, which is still too large.

Next I edited the root `CmakeLists.txt` file to strip the library to its bones. I replaced loaded opcodes with the bare neccessities, and removed all C++ Opcodes.

```
# list(APPEND libcsound_static_SRCS ${stdopcod_SRCS} ${externs_SRCS} ${gens_SRCS} 
# ${cs_pvs_ops_SRCS} ${physmod_SRCS}
# ${pitch_ops_SRCS}  ${oldpvoc_SRCS} ${scanops_SRCS} ${mp3in_SRCS} ${cppops_SRCS}
# ${hrtf_ops_SRCS} ${sock_ops_SRCS}  ${pitch_ops_SRCS}
# ${newgabops_SRCS}  ${oldpvoc_SRCS} ${vbap_ops}
# )

# We only want the bare essentials for a synth
list(APPEND libcsound_static_SRCS 
    Opcodes/ugens1.c     # Basic Oscillators (oscil, etc)
    Opcodes/ugens2.c     # Basic Filters
    Opcodes/ugens3.c     # Envelopes
)

# if(NOT BUILD_PLUGINS)
#  list(APPEND libcsound_SRCS ${cppops_SRCS} ${scanops_SRCS} ${physmod_SRCS}
#  ${mp3in_SRCS} ${hrtf_ops_SRCS} ${sock_ops_SRCS} ${externs_SRCS}
#  ${gens_SRCS} ${cs_pvs_ops_SRCS} ${newgabops_SRCS}  ${oldpvoc_SRCS}
#  ${pitch_ops_SRCS} ${vbap_ops} ${stdopcod_SRCS}
# )
#  list(APPEND libcsound_CFLAGS -DCS_INTERNAL)
# endif()
```

Building this version results in a 1.7MB `libcsound.a` file, and building the program with this file results in 830KB of flash used.

This means there is 200KB of flash left for any additional code, which is plenty.

However, there is a long list of errors that is `csmodule.c` complaining that it can't find references to opcodes we pruned.

Instead of having `csmodule.c` initialize all the opcoees, I will comment out all of the opcodes that are causing errors in `csmodule.c` and initialize the bare minimum manually:

```
// #ifndef BUILD_PLUGINS
//   sfont_ModuleDestroy(csound);
// #endif

// ...

CS_NOINLINE int32_t csoundInitStaticModules(CSOUND *csound)
{
  int32_t     i;
  int32_t length;
  OENTRY  *opcodlst_n;
  const INITFN staticmodules[] = {
// #if defined(LINUX) || defined(__MACH__)
//     cpumeter_localops_init,
// #endif
// #if !(defined(__wasi__))
//     counter_localops_init,
//     system_localops_init,
// #ifndef NO_SERIAL_OPCODES
//     serial_localops_init,
// #endif
// #ifdef HAVE_SOCKETS
//     sockrecv_localops_init,
//     socksend_localops_init,
// #endif
// #endif  // !wasi
// #if defined(LINUX) || defined(__MACH__)
//     control_localops_init, urandom_localops_init,
// #endif
//     scnoise_localops_init, afilts_localops_init,
//     mp3in_localops_init, hrtferX_localops_init,
//     hrtfearly_localops_init, hrtfreverb_localops_init,
//     bformdec2_localops_init, babo_localops_init,
//     bilbar_localops_init, vosim_localops_init,
//     compress_localops_init,  pinker_localops_init,
//     squinewave_localops_init,  eqfil_localops_init,
//     hrtfopcodes_localops_init, lufs_localops_init,
//     sterrain_localops_init,date_localops_init,
//     liveconv_localops_init, gamma_localops_init,
//     wpfilters_localops_init, gendy_localops_init,
//     phisem_localops_init, physmod_localops_init,
//     framebuffer_localops_init, cell_localops_init,
//     exciter_localops_init, buchla_localops_init,
//     select_localops_init, platerev_localops_init,
//     sequencer_localops_init,grain4_localops_init,
//     loscilx_localops_init, pan2_localops_init,
//     minmax_localops_init, vaops_localops_init,
//     ugakbari_localops_init, harmon_localops_init,
//     pitchtrack_localops_init, partikkel_localops_init,
//     shape_localops_init, tabsum_localops_init,
//     crossfm_localops_init, pvlock_localops_init,
//     fareyseq_localops_init,  paulstretch_localops_init,
//     tabaudio_localops_init,  scoreline_localops_init,
//     modmatrix_localops_init, ambicode1_localops_init,
    //arrayvars_localops_init, zak_localops_init,
    // scugens_localops_init, emugens_localops_init,
    // pvoc_localops_init, spectra_localops_init,
    // vbap_localops_init,
    NULL };

  const INITFN2 staticmodules2[] = {
    //stdopc_ModuleInit,
//     newgabopc_ModuleInit,
//     pvsopc_ModuleInit,
//     sfont_ModuleCreate,
//     sfont_ModuleInit,
//     csoundModuleInit_ampmidid,
//     csoundModuleCreate_mixer,
//     csoundModuleInit_mixer,
//     csoundModuleInit_doppler,
// #if !defined(BARE_METAL) && !defined(__wasi__)
//     csoundModuleInit_ftsamplebank,
// #endif
//     csoundModuleInit_signalflowgraph,
//     arrayops_init_modules,
//     lfsr_init_modules,
//     pvsops_init_modules,
//     trigEnv_init_modules,
//     csoundModuleInit_fractalnoise,
//     scansyn_init_,
//     scansynx_init_,
    NULL
  };

  const FGINITFN fgentab[] = {
    // ftest_fgens_init, farey_fgens_init, quadbezier_fgens_init,
    // padsyn_fgen_init, mp3in_fgen_init, 
    NULL };
// ...
```

Building this library and building the program creates an .elf file successfully.

Now I will implement Csound in my program with manual initializations:

```cpp
CSOUND* cs = nullptr;

const char* csd_template = R"csd(
    <CsoundSynthesizer>
    <CsOptions>
    -n
    </CsOptions>
    <CsInstruments>
    sr = 48000
    ksmps = %d 
    nchnls = 2
    0dbfs = 1.0
    
    giSine ftgen 1, 0, 16384, 10, 1
    
    instr 1
      aout oscil 0.5, 440, 1
      outs aout, aout
    endin
    </CsInstruments>
    <CsScore>
    i 1 0 3600
    </CsScore>
    </CsoundSynthesizer>
    )csd";

void csound_init_app() {
    char final_csd[2048];
    
    // Inject the macro NUM_FRAMES into the string
    snprintf(final_csd, sizeof(final_csd), csd_template, NUM_FRAMES);

    cs = csoundCreate(NULL, NULL);
    
    int result = csoundCompileCSD(cs, final_csd, 1, 0);
    
    if (result == 0) {
        csoundStart(cs);
    }
}

// ...

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (cs != nullptr) {
        int perform_status = csoundPerformKsmps(cs);
        
        if (perform_status == 0) {
            // 3. Get the calculated samples (floats)
            const MYFLT* spout = csoundGetSpout(cs);
            
            // 4. Convert and copy into the DMA buffer
            // Csound generates interleaved audio [L, R, L, R...]
            for (int i = 0; i < 64; i++) {
                // Convert float -1.0..1.0 to signed 16-bit
                float sample = spout[i];
                
                // Hard clipping for safety
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                
                buffer[i + (NUM_FRAMES * 2)] = (int16_t)(sample * 32767.0f);
            }
        }
    }
}
```

This code builds successfully, but when debugging on the board, ends in an immediate Hard Fault.

Implementing csoundCreate caused an immediate Hard Fault; commenting it out allowed the board to boot, proving the linker only pulls in the crashing code (C++ global constructors) when the API is referenced.

Static vs. Dynamic RAM: Clarified that Csound’s footprint is primarily dynamic (Heap), meaning it doesn't show up in the compiler’s RAM report but must be reserved in the .ld file.

The "Ghost Phase" (Pre-Main): Confirmed the crash occurs during C-Runtime startup; breakpoints at the first line of main() were never hit.

Memory Layout Verification: Confirmed the Linker Script correctly places _estack at 0x20020000 and _ebss at 0x200031d8, leaving ~110KB of free RAM.

The "Lockup" Signature: Observed SP = 0xFFFFFF98 and LR = 0xFFFFFFE9 during faults, indicating a "Double Fault" or "Stacking Failure" where the CPU failed to save its state during an exception.

FPU Configuration: Identified that Csound (Hard Float) was crashing because the FPU wasn't enabled. Manually added assembly to the Reset_Handler in startup_stm32f407xx.s to enable CP10/CP11.

POSIX System Stubs: Recognized that Csound requires standard C functions (_sbrk, _open, _gettimeofday) and implemented csound_stub.cpp to provide them.

Build System Conflict Resolution: Resolved "Multiple Definition" errors by excluding the standard CubeMX-generated sysmem.c and syscalls.c in favor of our custom stubs.

Stub Expansion & C-Linkage: Successfully generated the .elf binary after adding missing stubs (_stat, _times, _unlink) and forcing extern "C" linkage to satisfy the linker.

Recursive Hard Fault Analysis: Observed _calloc_r and memset in the call stack, confirming the crash happens when a C++ constructor tries to allocate and zero-out memory via _sbrk.

Heap/Stack Boundary Test: Increased the Linker Script heap size to 80KB to ensure the fault wasn't a simple internal Newlib check for insufficient space.

Thumb-Mode Alignment Check: Discovered via arm-none-eabi-nm that _sbrk was located at an even address (0x0800416c), which triggers a Hard Fault on Cortex-M4 because even addresses imply an unsupported switch to ARM mode.

Current Failure Point: Despite enabling the FPU and providing stubs, the system still faults before reaching main(), likely due to the "even address" jump or a memory alignment violation during the memset of a global object.
