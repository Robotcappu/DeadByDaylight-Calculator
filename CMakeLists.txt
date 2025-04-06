cmake_minimum_required(VERSION 3.16)

project(Bloodpoints_Calculator LANGUAGES CXX)

# Setze den C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setze Standard-Build-Typ auf Release, falls keiner angegeben
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

# Compiler-spezifische Flags
if (MSVC)
    message(STATUS "Configuring for MSVC...")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
else()
    message(STATUS "Configuring for GCC/Clang...")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Windows-spezifische Anpassungen
if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -municode -static -static-libgcc -static-libstdc++")
    endif()
endif()

# Header-Suchpfade
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/core
    ${CMAKE_SOURCE_DIR}/modules/settings
    ${CMAKE_SOURCE_DIR}/modules/utilities
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/json
)

# Sammle alle Quellcodes automatisch
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/*.cpp
)

# Unnötige .cpp-Dateien ignorieren
list(FILTER SOURCES EXCLUDE REGEX ".*/(build.*|CMakeFiles)/.*")

# Ressourcenfile für Windows einbinden
if (WIN32)
    list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/resource.rc)
endif()

# Erzeuge das Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Setze das Subsystem korrekt für GUI-Programme
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
endif()

# Windows Libraries linken
if (WIN32)
    target_link_libraries(${PROJECT_NAME} d3d9 dwmapi gdi32)
endif()
