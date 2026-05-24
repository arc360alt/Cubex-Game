FILE(GLOB raknetCPPSources ./RaknetSources/*.cpp)

# Use same 32/64 build mode as root caller if set
if(NOT DEFINED BUILD_32BIT)
	set(BUILD_32BIT OFF CACHE BOOL "RakNet 32-bit build")
endif()

if(BUILD_32BIT)
	set(CMAKE_C_FLAGS "-m32")
	set(CMAKE_CXX_FLAGS "-m32")
else()
	set(CMAKE_C_FLAGS "-m64")
	set(CMAKE_CXX_FLAGS "-m64")
endif()

add_library(raknet ${raknetCPPSources})
