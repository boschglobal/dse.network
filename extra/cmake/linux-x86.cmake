# Target: Linux x86

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_FLAGS -mx32)
set(CMAKE_CXX_FLAGS -mx32)
set(CDEF_PLATFORM_OS linux)         # Available via dse/clib/platform.h.
set(CDEF_PLATFORM_ARCH x86)         # Available via dse/clib/platform.h.


# Enable CCache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
