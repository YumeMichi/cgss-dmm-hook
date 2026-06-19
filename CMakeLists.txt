cmake_minimum_required(VERSION 3.20)

project(cgss_http_hook LANGUAGES C CXX RC ASM_MASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(version SHARED
    src/proxy.cpp
    src/hook.cpp
    src/log.cpp
    src/il2cpp_symbols.cpp
    src/version.rc
    src/version.def
    deps/minhook/src/buffer.c
    deps/minhook/src/hook.c
    deps/minhook/src/trampoline.c
    deps/minhook/src/hde/hde64.c
)

target_include_directories(version PRIVATE
    src
    deps/minhook/include
    deps/minhook/src
    deps/minhook/src/hde
)

target_compile_definitions(version PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(version PRIVATE
    user32
    kernel32
)

set_target_properties(version PROPERTIES
    OUTPUT_NAME "version"
    PREFIX ""
)

if(MINGW)
    target_compile_options(version PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>"
        "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
    )
    target_link_options(version PRIVATE
        "-static-libgcc"
        "-static-libstdc++"
    )
endif()
