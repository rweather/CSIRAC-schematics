cmake_minimum_required(VERSION 3.5)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CheckIncludeFiles)
include(CheckLibraryExists)

# Definitions for building ".cna" files with the CSIRAC assembler.
# Note: Turning "CNA" into a real cmake language isn't quite working yet.
set(CSIRAC_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/library)
set(CSIRAC_ASSEMBLER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/tools/assembler/assemble.sh)
set(CSIRAC_ASSEMBLER ${CMAKE_CURRENT_BINARY_DIR}/tools/assembler/csirac-assembler)
set(ENV{CSIRAC_ASSEMBLER} ${CSIRAC_ASSEMBLER})

# Set the project name and version number.
project(csirac-tools VERSION 0.1.0 LANGUAGES C CNA)

# Set up the type of build and the compiler flags to use.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    #set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS "-Wall -Wextra ${CMAKE_C_FLAGS} -DCSIRAC_LIBRARY_DIR=\\\"${CMAKE_INSTALL_PREFIX}/share/csirac/library\\\"")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Require the c99 standard to compile C code.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include the CSIRAC source build rules.
include(CSIRAC)

# Set up the main include directory.
include_directories(tools)

# Add the subdirectories.
add_subdirectory(tools)
add_subdirectory(library)
add_subdirectory(examples)
