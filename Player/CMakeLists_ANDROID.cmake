
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

set(TARGET_OS_ANDROID "android")
set(OMAF_CMAKE_TARGET_OS ${TARGET_OS_ANDROID})

add_definitions(-DTARGET_OS_ANDROID)
message("---------------------")
if (BUILD_SHARED)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden") #by default hide all symbols, only export explicitly (we do NOT want to publicize our internal methods/data)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,--build-id")    #store build-id/hash so androidstudio can match stripped binary on device with fully symbolized one on pc
endif()

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    #add stuff that custom toolchain added..
    message("Using cmake pre-installed toolchain")
    add_definitions(-DANDROID)
    if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")                
        #HRMPH our custom one sets these different flags for arm8...
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer") #make sure all stack frames have valid stackframes.
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-strict-aliasing" )   #dont allow compiler to assume anything about casts.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")  #breaks stackframes (ie. debugging), since not all functions need it, BUT registers can be used for other purposes which might make the functions faster!
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fstrict-aliasing")     #allow the compiler to assume strictest aliasing rules. "In particular, an object of one type is assumed never to reside at the same address as an object of a different type, unless the types are almost the same."
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funswitch-loops")      #Move branches with loop invariant conditions out of the loop, with duplicates of the loop on both branches (modified according to result of the condition). 
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -finline-limit=300")    #limit inlined functions to max 300 "insns"
    else()
        #NOTE: for some reason debug&release both set no-strict-aliasing. (on all other ABIs release sets strict-aliasing!)
        #which does look like a bug in the old toolchain...
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer") #make sure all stack frames have valid stackframes.
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-strict-aliasing")    #dont allow compiler to assume anything about casts.
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -finline-limit=64")       #limit inlined functions to max 64 "insns"
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")  #breaks stackframes (ie. debugging), since not all functions need it, BUT registers can be used for other purposes which might make the functions faster!
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-strict-aliasing")  #dont allow compiler to assume anything about casts.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -finline-limit=64")     #limit inlined functions to max 64 "insns"
        #for some reason our custom toolchain built arm7 debugs with -marm....     although it is recommended/default to use thumb :)        
        #HRMPH, this does not seem to work. i guess this needs to be given in commandline..
        #if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        #    set(CMAKE_ANDROID_ARM_MODE ON)
        #endif()
    endif()
    
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG -D_DEBUG")                        #zero optimizations, and debug macros.
    #set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -g")                                         #this ones set by default (generate debug symbols)
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -Wa,--noexecstack")                           #umm, we dont use assembler, so this is completely unnecessary. (gives --noexecstack to assembler)
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -fsigned-char")                               #questionable, we should actually make sure our code uses explicit signed/unsigned char!
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -Wno-psabi")                                  #unnecessary, only needed with some GCC versions, warns about ABI differences
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -fdata-sections")                             #"function level linking" (this might not be a good idea. all constant data is put in their own sections, and, possibly, later removed. data accesses will be slower.)
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -ffunction-sections")                         #"function level linking" (all functions are added to own sections, and, possibly, later removed. technically good, but might cause unnecessary redirection.)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,--no-undefined")                #Force linker error for unresolved symbols! (good)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,-allow-shlib-undefined")        #Allow unresolved symbols for .so linking... apparently it's a good thing? (allows use of non runtime matching .so's?) (this is apparently the default for GCC, and not needed here)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,--gc-sections")                 #combined with function level linking, remove un-used code/data.
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,-z,noexecstack")                #mark stack as non-exec. security?
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,-z,relro")                      #read only reloc, security?
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -Wl,-z,now")                        #force .so runtime linking during start up, and not lazily. (ie. link shared libraries during load and not during function calls.)


    #set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")                          #this ones set by default (level 3 optimization and NDEBUG macro)
    #set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -g")                                    #this ones set in the sources/cmakelists.txt (generate debug symbols) (should move here?)
    set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -Wa,--noexecstack")                      #umm, we dont use assembler, so this is completely unnecessary. (gives --noexecstack to assembler)
    set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -fsigned-char")                         #questionable, we should actually make sure our code uses explicit signed/unsigned char!
    set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -Wno-psabi")                            #unnecessary, only needed with some GCC versions, warns about ABI differences
    set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -fdata-sections")                       #"function level linking" (this might not be a good idea. all constant data is put in their own sections, and, possibly, later removed. data accesses will be slower.)
    set(CMAKE_CXX_FLAGS_RELEASE             "${CMAKE_CXX_FLAGS_RELEASE} -ffunction-sections")                   #"function level linking" (all functions are added to own sections, and, possibly, later removed. technically good, but might cause unnecessary redirection.)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--no-undefined")          #Force linker error for unresolved symbols! (good)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-allow-shlib-undefined")  #Allow unresolved symbols for .so linking... apparently it's a good thing? (allows use of non runtime matching .so's?) (this is apparently the default for GCC, and not needed here)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")           #combined with function level linking, remove un-used code/data/bss sections.
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,noexecstack")          #mark stack as non-exec. security?
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,relro")                #read only reloc, security?
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE   "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,now")                  #force .so runtime linking during start up, and not lazily. (ie. link shared libraries during load and not during function calls.)
    
else()
    message("Using custom toolchain ${CMAKE_TOOLCHAIN_FILE} !!")
endif()

set(OMAF_ROOT ..)
set(OMAF_COMMON_DEPS
    log
    android
    EGL
    OpenMAXAL
    mediandk
    GLESv3
    )

if (ENABLE_OPENSL)
    set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} OpenSLES)
endif()

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MP4VR_LIB_DIR ${CMAKE_SOURCE_DIR}/${OMAF_ROOT}/Mp4/lib/${OMAF_CMAKE_TARGET_OS}/debug/${ANDROID_ABI})
ELSE()
    set(MP4VR_LIB_DIR ${CMAKE_SOURCE_DIR}/${OMAF_ROOT}/Mp4/lib/${OMAF_CMAKE_TARGET_OS}/release/${ANDROID_ABI})
ENDIF()

set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} ${MP4VR_LIB_DIR}/libmp4vr_shared.so)

#need to add ABI on android
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${ANDROID_ABI})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${ANDROID_ABI})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ANDROID_ABI})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_PDB_OUTPUT_DIRECTORY}/${ANDROID_ABI})

set(libprefix "lib")
set(libsuffix "a")
set(dashsuffix "so")
