cmake_minimum_required(VERSION 3.17.3)
project(swamp_script_scope C)

set(CMAKE_C_STANDARD 11)
add_compile_definitions(_POSIX_C_SOURCE=200112L TORNADO_OS_LINUX CONFIGURATION_DEBUG TYRAN_CONFIGURATION_DEBUG)

set(DEPS ../../deps/)

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "../deps/*/src/*.c"
        )


# Remove iOS and SDL (including Webassembly)
list(FILTER deps_src EXCLUDE REGEX ".*sdl_main.c" )
list(FILTER deps_src EXCLUDE REGEX ".*webassembly/main.c" )
list(FILTER deps_src EXCLUDE REGEX ".*src/examples/main.c" )
list(FILTER deps_src EXCLUDE REGEX ".*src/example/main.c" )
list(FILTER deps_src EXCLUDE REGEX ".*src/example/app.c" )
list(FILTER deps_src EXCLUDE REGEX ".*src/platform/ios/.*" )
list(FILTER deps_src EXCLUDE REGEX ".*src/platform/posix/.*" )
list(FILTER deps_src EXCLUDE REGEX ".*src/platform/pulseaudio/.*" )
list(FILTER deps_src EXCLUDE REGEX ".*src/platform/webassembly/.*" )
list(FILTER deps_src EXCLUDE REGEX ".*src/platform/sdl/.*" )


file(GLOB_RECURSE lib_src FOLLOW_SYMLINKS
        "*.c"
        )

add_library(swamp_script_scope
        ${lib_src}
        )

if(WIN32)
    message("detected windows!")
    target_compile_definitions(swamp_script_scope PRIVATE TORNADO_OS_WINDOWS)
endif()

target_include_directories(swamp_script_scope PUBLIC ${DEPS}piot/clog/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}piot/tiny-libc/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}piot/imprint/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}piot/monotonic-time-c/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}swamp/runtime-c/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}swamp/typeinfo-c/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}swamp/dump-c/src/include)
target_include_directories(swamp_script_scope PUBLIC ${DEPS}swamp/state-c/src/include)
target_include_directories(swamp_script_scope PUBLIC ../include)

target_link_libraries(swamp_script_scope m)
target_link_libraries(swamp_script_scope -fsanitize=address)
