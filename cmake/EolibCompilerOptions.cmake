# Defines the eolib_compiler_options interface target, which applies the project's warning policy to internal targets.
# It is intentionally not propagated to consumers of eolib.

add_library(eolib_compiler_options INTERFACE)

if(MSVC)
    target_compile_options(eolib_compiler_options INTERFACE /W4 /permissive- /utf-8)
    if(EOLIB_WARNINGS_AS_ERRORS)
        target_compile_options(eolib_compiler_options INTERFACE /WX)
    endif()
else()
    target_compile_options(eolib_compiler_options INTERFACE -Wall -Wextra -Wpedantic)
    if(EOLIB_WARNINGS_AS_ERRORS)
        target_compile_options(eolib_compiler_options INTERFACE -Werror)
    endif()
endif()
