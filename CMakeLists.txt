# This file is mostly the property of Kaiser and Altazimuth, horribly cobbled together by SaladBadger
#
# As part of the Kex Engine project, it is released by Kaiser under the GPL version 2

cmake_minimum_required(VERSION 3.11)
project(ChocolateDescent LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

cmake_policy(SET CMP0076 OLD)

set_property(GLOBAL PROPERTY KEX_ENGINE_SRCS "")
set_property(GLOBAL PROPERTY SOURCE_GROUPS "")

set(METADATA_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Base path for project")
set(BUILD_EDITOR OFF CACHE BOOL "Build the game editor")

# The following macros are provided by Kaiser and Altazimuth, from the Kex Engine project.
# ---------------------------------------------------------------------------------
#
macro(add_file)
    get_property(SRCS GLOBAL PROPERTY KEX_ENGINE_SRCS)
    foreach(_src ${ARGN})
        list(APPEND SRCS "${_src}")
    endforeach()
    set_property(GLOBAL PROPERTY KEX_ENGINE_SRCS ${SRCS})
endmacro()

macro(add_sources)
    set(_current_source_group)
    set(_expecting_file FALSE)
    set(_expecting_group_name FALSE)
    
    foreach(ARG ${ARGN})
        if(${ARG} STREQUAL "SOURCE_GROUP")
            set(_expecting_group_name TRUE)
        elseif(_expecting_group_name)
            set(_current_source_group ${ARG})
            string(REPLACE " " "_" _current_source_group_var ${_current_source_group})
            list(FIND SOURCE_GROUPS ${_current_source_group_var} _group_index)    
            
            if(_group_index EQUAL -1)
                list(APPEND SOURCE_GROUPS ${_current_source_group_var})
                set(SOURCE_GROUP_${_current_source_group_var})
            endif()
            
            set(_expecting_group_name FALSE)
            set(_expecting_file TRUE)
        elseif(_expecting_file)
            if(NOT _current_source_group)
                message(FATAL_ERROR "No source group name defined")
            endif()
            
            list(APPEND SOURCE_GROUP_${_current_source_group_var} ${ARG})
            
            if(NOT ${_current_source_group} STREQUAL "Root")
                source_group(${_current_source_group} FILES ${ARG})
            else()
                source_group("" FILES ${ARG})
            endif()
            
            if(EXISTS ${ARG})
                add_file(${ARG})
            endif()
        endif()
    endforeach()
endmacro()


find_package(SDL2)
find_package(OPENAL)
find_package(FLUIDSYNTH)

add_subdirectory(2d)
add_subdirectory(3d)
add_subdirectory(cfile)
add_subdirectory(fix)
add_subdirectory(iff)
add_subdirectory(main_d1)
add_subdirectory(mem)
add_subdirectory(misc)
add_subdirectory(platform)
add_subdirectory(texmap)
add_subdirectory(ui)
add_subdirectory(vecmat)

get_property(PROJECT_FILES GLOBAL PROPERTY KEX_ENGINE_SRCS)

#[ISB] allegedly don't need to check platform. yay.
add_executable(ChocolateDescent WIN32 ${PROJECT_FILES}) #[ISB] sources added down the line... i guess?


#[ISB] so I somehow messed up Kaiser's really cool grouping macro and I honestly have no clue why, so do this the hard way for now
source_group("2d" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/2d/.+")
source_group("3d" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/3d/.+")
source_group("cfile" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/cfile/.+")
source_group("fix" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/fix/.+")
source_group("iff" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/iff/.+")
source_group("main_d1" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/main_d1/.+")
source_group("mem" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/mem/.+")
source_group("misc" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/misc/.+")
source_group("platform" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/platform/.+")
source_group("platform\\fluidsynth" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/platform/fluidsynth/.+")
source_group("platform\\openal" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/platform/openal/.+")
source_group("platform\\sdl" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/platform/sdl/.+")
source_group("platform\\win32" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/platform/win32/.+")
source_group("ui" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/ui/.+")
source_group("texmap" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/texmap/.+")
source_group("vecmat" REGULAR_EXPRESSION "${METADATA_BASE_PATH}/vecmat/.+")

#[ISB] if we got SDL2, set the USE_SDL definition
if (SDL2_FOUND)
    target_compile_definitions(ChocolateDescent PUBLIC USE_SDL)
endif()

if (OPENAL_FOUND)
    target_compile_definitions(ChocolateDescent PUBLIC USE_OPENAL)
endif()

if (FLUIDSYNTH_FOUND)
    target_compile_definitions(ChocolateDescent PUBLIC USE_FLUIDSYNTH)
endif()

include_directories(.) #When I asked about it, I was told once that "All cmake projects are bad." I think this is evidence of it.

if (SDL2_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${SDL2_LIBRARIES}
    )

    target_include_directories(ChocolateDescent SYSTEM PRIVATE
        ${SDL2_INCLUDE_DIRS}
    )
endif()

if (SDL2_MAIN_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${SDL2_MAIN_LIBRARIES}
    )
endif()

if (OPENAL_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${OPENAL_LIBRARIES}
    )

    target_include_directories(ChocolateDescent SYSTEM PRIVATE
        ${OPENAL_INCLUDE_DIRS}
    )
endif()

if (FLUIDSYNTH_FOUND)
    target_link_libraries(ChocolateDescent PRIVATE
        ${FLUIDSYNTH_LIBRARIES}
    )

    target_include_directories(ChocolateDescent SYSTEM PRIVATE
        ${FLUIDSYNTH_INCLUDE_DIRS}
    )
endif()

#Need some additional definitions in release configuration.
target_compile_definitions(ChocolateDescent PUBLIC "$<$<CONFIG:RELEASE>:RELEASE>")