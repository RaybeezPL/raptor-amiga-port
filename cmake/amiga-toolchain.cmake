## ============================================================================
## Raptor - Amiga (m68k / AmigaOS 3.x) CMake cross toolchain
##
## The Amiga port builds with the in-tree SDL2 stub (src/amiga), so no external
## SDL2 is required or searched here.  This file contains NO hard-coded absolute
## compiler/SDK paths; everything is supplied through CMake cache variables or
## -D arguments:
##
##   -DRAPTOR_AMIGA_PREFIX=<prefix>   prefix of the m68k AmigaOS cross
##                                    compilers (default: m68k-amigaos)
##   -DAMIGA_SDK_PREFIX=<path>        optional root of an AmigaOS SDK/toolchain
##                                    (e.g. /opt/amiga) used for find_root_path;
##                                    empty by default
##   -DCMAKE_C_COMPILER=<path>        optional explicit C compiler override
##   -DCMAKE_CXX_COMPILER=<path>      optional explicit C++ compiler override
##
## Typical run from the repo root:
##   cmake -S . -B build.amiga.cmake -DCMAKE_TOOLCHAIN_FILE=cmake/amiga-toolchain.cmake
## ============================================================================

set(RAPTOR_AMIGA_PREFIX "m68k-amigaos" CACHE STRING
    "Prefix of the m68k AmigaOS cross compilers")

set(AMIGA_SDK_PREFIX "" CACHE PATH
    "Optional root of an AmigaOS SDK/toolchain (e.g. /opt/amiga) for find_root_path")

# Target OS / CPU (this also marks the build as a cross build and prevents
# CMake from trying to run the produced binaries on the host).
set(CMAKE_SYSTEM_NAME "AmigaOS")
set(CMAKE_SYSTEM_PROCESSOR "m68k")

# Compiler selection.  Only set a value if the caller did not already provide
# an explicit one.
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${RAPTOR_AMIGA_PREFIX}-gcc" CACHE FILEPATH "AmigaOS C compiler")
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${RAPTOR_AMIGA_PREFIX}-g++" CACHE FILEPATH "AmigaOS C++ compiler")
endif()
if(NOT CMAKE_ASM_COMPILER)
    set(CMAKE_ASM_COMPILER "${RAPTOR_AMIGA_PREFIX}-as" CACHE FILEPATH "AmigaOS assembler")
endif()

# Toolchain utilities.
set(CMAKE_AR        "${RAPTOR_AMIGA_PREFIX}-ar")
set(CMAKE_RANLIB    "${RAPTOR_AMIGA_PREFIX}-ranlib")
set(CMAKE_STRIP     "${RAPTOR_AMIGA_PREFIX}-strip")
set(CMAKE_NM        "${RAPTOR_AMIGA_PREFIX}-nm")
set(CMAKE_OBJCOPY   "${RAPTOR_AMIGA_PREFIX}-objcopy")
set(CMAKE_OBJDUMP   "${RAPTOR_AMIGA_PREFIX}-objdump")

# Optional SDK root: when given, restrict searches to it so the build only
# finds headers/libraries of the AmigaOS SDK.
if(AMIGA_SDK_PREFIX)
    set(CMAKE_FIND_ROOT_PATH "${AMIGA_SDK_PREFIX}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()
