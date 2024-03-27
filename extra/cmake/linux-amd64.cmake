# Target: Linux AMD64

set(CMAKE_SYSTEM_NAME Linux)
set(CDEF_PLATFORM_OS linux)         # Available via dse/clib/platform.h.
set(CDEF_PLATFORM_ARCH amd64)       # Available via dse/clib/platform.h.


# Enable CCache if available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
