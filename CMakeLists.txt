cmake_minimum_required(VERSION 3.10)

set(EXECUTABLE_OUTPUT_PATH "bin")
set(CMAKE_CXX_FLAGS "-O3")
SET(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-O3")
SET(CMAKE_C_STANDARD 11)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

project(raptor)

# ===========================================================================
# Amiga port detection.
#
# The Amiga port (AmigaOS 3.x / 68k) uses its OWN minimal SDL2 stub layer in
# src/amiga/ (SDL.h -> amiga_sdl_stubs.h, enabled with -DUSE_SDL_STUBS), so an
# Amiga build must NOT call find_package(SDL2) and must NOT link SDL2::SDL2 /
# SDL2::SDL2main.  SDL2 is required only for the host (Windows/Linux/macOS/
# Android) builds.
#
# Detection (a cross toolchain file should set CMAKE_SYSTEM_NAME, e.g.
# "AmigaOS"):
#   - CMAKE_SYSTEM_NAME       matches "Amiga", or
#   - CMAKE_SYSTEM_PROCESSOR  matches "m68k", or
#   - explicit -DRAPTOR_AMIGA=ON.
# ===========================================================================
if(NOT DEFINED RAPTOR_AMIGA)
    if(CMAKE_SYSTEM_NAME MATCHES "Amiga" OR CMAKE_SYSTEM_PROCESSOR MATCHES "m68k")
        set(RAPTOR_AMIGA TRUE)
    else()
        set(RAPTOR_AMIGA FALSE)
    endif()
endif()

# SDL2 discovery below is host-only.  The Amiga port skips it entirely and
# relies on its in-tree stub (see the AMIGA BUILD branch further down).
if(NOT RAPTOR_AMIGA)
if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        SDL
        URL            https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.10.zip
        SOURCE_DIR     ${CMAKE_BINARY_DIR}/include/SDL2
        )
    set(BUILD_SHARED_LIBS ON)
    set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
    set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL)
    set(SDL_ACTIVITY_FOLDER app/src/main/java/org/libsdl/app/)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})
    file(COPY ${CMAKE_BINARY_DIR}/include/SDL2/android-project/${SDL_ACTIVITY_FOLDER} DESTINATION ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/include/TinySoundFont/LICENSE DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses)
    file(RENAME ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses/LICENSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses/LICENSE.TSF)
    file(COPY ${CMAKE_SOURCE_DIR}/pkg/android/license/LICENSE DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/licenses)
elseif(WIN32)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    if(MSVC)
        set(SDL2_DIR "include/SDL2-devel-2.28.2-VC/SDL2-2.28.2/cmake")
    else()
        set(SDL2_DIR "include/SDL2-devel-2.28.2-mingw/SDL2-2.28.2/cmake")
    endif()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()
endif()

# The textscreen library (setup-program UI) is a host-only third-party
# component.  It is added only when the directory is actually present in the
# tree (the Amiga port and the bare checkout do not ship it).
include_directories(./include/TinySoundFont/)

if(NOT RAPTOR_AMIGA)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/textscreen/include/CMakeLists.txt)
        include_directories(./include/textscreen/include/)
        add_subdirectory(./include/textscreen/include)
    else()
        message(WARNING "textscreen subdirectory not found - 'raptorsetup' and any textscreen-dependent linking will be skipped.")
    endif()
endif()

# ---------------------------------------------------------------------------
# AMIGA BUILD - uses the in-tree SDL2 stub.  NO external/system SDL2.
# Source set and definitions match Makefile.amiga.
# ---------------------------------------------------------------------------
if(RAPTOR_AMIGA)
    set(AMIGA_SOURCE_FILES
        src/anims.cpp
        src/bonus.cpp
        src/demo.cpp
        src/dspapi.cpp
        src/enemy.cpp
        src/eshot.cpp
        src/flame.cpp
        src/fx.cpp
        src/gfxapi.cpp
        src/gfxapi_a.cpp
        src/glbapi.cpp
        src/gssapi.cpp
        src/help.cpp
        src/i_lastscr.cpp
        src/i_oplmusic.cpp
        src/i_video.cpp
        src/imsapi.cpp
        src/input.cpp
        src/intro.cpp
        src/joyapi.cpp
        src/kbdapi.cpp
        src/loadsave.cpp
        src/m_misc.cpp
        src/movie.cpp
        src/movie_a.cpp
        src/mpucamd.cpp
        src/mpumhi.cpp
        src/mputsf.cpp
        src/musapi.cpp
        src/objects.cpp
        src/dbopl.cpp
        src/opl3dbopl.cpp
        src/ptrapi.cpp
        src/ptrapi_a.cpp
        src/rap.cpp
        src/shadow.cpp
        src/shots.cpp
        src/store.cpp
        src/swdapi.cpp
        src/amiga/amiga_cfg.cpp
        src/amiga/amiga_stubs_impl.cpp
        src/tile.cpp
        src/tile_a.cpp
        src/vmemapi.cpp
        src/windows.cpp
        )

    add_executable(${PROJECT_NAME} ${AMIGA_SOURCE_FILES})

    target_include_directories(${PROJECT_NAME} PRIVATE
        src
        src/amiga
        include/TinySoundFont
        )

    # The Amiga stub SDL layer plus the native AmigaOS code paths.
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        AMIGA
        __AMIGA__
        USE_SDL_STUBS
        NDEBUG
        )

    # Base AmigaOS C/C++ libraries (same as Makefile.amiga: -lm -lstdc++).
    target_link_libraries(${PROJECT_NAME} PRIVATE m stdc++)

    # -----------------------------------------------------------------------
    # AmigaOS CRT / CPU flags matching Makefile.amiga.
    # The essential -noixemul selects the libnix (non-ixemul, newlib) CRT,
    # which is what supplies access() and the AmigaOS syscall hooks
    # (_gettimeofday etc.).  CPU/ABI flags are cache variables so an FPU-less
    # (NOFPU) build can override them, exactly like Makefile.amiga NOFPU=1.
    # -----------------------------------------------------------------------
    set(RAPTOR_AMIGA_CPU_FLAGS "-m68060 -m68881" CACHE STRING
        "Amiga CPU/ABI flags (see Makefile.amiga; empty for none)")
    set(RAPTOR_AMIGA_LINK_FLAGS "-m68060 -m68881" CACHE STRING
        "Amiga link flags (see Makefile.amiga; empty for none)")
    separate_arguments(RAPTOR_AMIGA_CPU_FLAGS_LIST UNIX_COMMAND "${RAPTOR_AMIGA_CPU_FLAGS}")
    separate_arguments(RAPTOR_AMIGA_LINK_FLAGS_LIST UNIX_COMMAND "${RAPTOR_AMIGA_LINK_FLAGS}")

    target_compile_options(${PROJECT_NAME} PRIVATE
        -noixemul
        ${RAPTOR_AMIGA_CPU_FLAGS_LIST}
        -fomit-frame-pointer
        -fno-exceptions
        -fno-rtti
        )
    target_link_options(${PROJECT_NAME} PRIVATE
        -noixemul
        ${RAPTOR_AMIGA_LINK_FLAGS_LIST}
        )

    message(STATUS "Raptor: building the Amiga port with the in-tree SDL2 stub (external SDL2 NOT used).")

else()


set(SOURCE_FILES
    src/tile.cpp
    src/gssapi.cpp
    src/objects.cpp
    src/tile_a.cpp
    src/windows.cpp
    src/flame.cpp
    src/i_lastscr.cpp
    src/joyapi.cpp
    src/kbdapi.cpp
    src/glbapi.cpp
    src/i_video.cpp
    src/musapi.cpp
    src/movie_a.cpp
    src/ptrapi_a.cpp
    src/swdapi.cpp
    src/imsapi.cpp
    src/store.cpp
    src/i_oplmusic.cpp
    src/bonus.cpp
    src/anims.cpp
    src/shadow.cpp
    src/movie.cpp
    src/m_misc.cpp
    src/shots.cpp
    src/opl3.cpp
    src/eshot.cpp
    src/gfxapi_a.cpp
    src/dspapi.cpp
    src/vmemapi.cpp
    src/rap.cpp
    src/demo.cpp
    src/gfxapi.cpp
    src/intro.cpp
    src/help.cpp
    src/input.cpp
    src/fx.cpp
    src/ptrapi.cpp
    src/loadsave.cpp
    src/enemy.cpp
    src/mpuwinmm.cpp
    src/mputsf.cpp
    src/mpualsa.cpp
    src/mpucorea.cpp
    src/mpucorem.cpp
    )

set(SOURCE_SETUP_FILES
    src/setup/input.cpp
    src/setup/main.cpp
    src/setup/music.cpp
    src/setup/prefapi.cpp
    src/setup/sound.cpp
    src/setup/txt_joyinput.cpp
    src/setup/txt_keyinput.cpp
    src/setup/txt_mouseinput.cpp
    )

if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_library( ${PROJECT_NAME} SHARED ${SOURCE_FILES})   
    target_link_libraries(raptor
        textscreen
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(ALSA REQUIRED)
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor
        textscreen
        asound
        SDL2::SDL2main
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(AUDIO_TOOLBOX AudioToolbox)
    find_library(AUDIO_UNIT AudioUnit)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(COREMIDI_LIB CoreMIDI)
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor ${AUDIO_TOOLBOX} ${AUDIO_UNIT} ${FOUNDATION_LIBRARY} ${COREMIDI_LIB}
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/resource.rc)
        target_sources(raptor PRIVATE rsrc/resource.rc)
    endif()
    target_link_libraries(raptor
        winmm.lib
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )
    if(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo" OR ${CMAKE_BUILD_TYPE} MATCHES "Release")
        if(MSVC)
            target_link_options(raptor PRIVATE /subsystem:windows /ENTRY:mainCRTStartup)
        else()
            target_link_options(raptor PRIVATE -mwindows)
        endif()
    elseif(NOT MSVC)
        target_link_options(raptor PRIVATE -mconsole)
    endif()
else()
    add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )
    target_link_libraries(raptor
        textscreen
        SDL2::SDL2main
        SDL2::SDL2
        )
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Android")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/setup)
        add_executable( raptorsetup ${SOURCE_SETUP_FILES} )
        if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/setup/resource.rc)
            target_sources(raptorsetup PRIVATE rsrc/setup/resource.rc)
        endif()
        target_link_libraries(raptorsetup
            textscreen
            SDL2::SDL2main
            SDL2::SDL2
            )
        if(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo" OR ${CMAKE_BUILD_TYPE} MATCHES "Release")
            if(MSVC)
                target_link_options(raptorsetup PRIVATE /subsystem:windows /ENTRY:mainCRTStartup)
            elseif(WIN32)
                target_link_options(raptorsetup PRIVATE -mwindows)
            endif()
        elseif(WIN32 AND NOT MSVC)
            target_link_options(raptorsetup PRIVATE -mconsole)
        endif()
    endif()
endif()
endif()

