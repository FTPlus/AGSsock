cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project("AGS Sockets")

add_library(agssock SHARED
	src/agsplugin.cpp
	src/Socket.cpp
	src/API.cpp
	src/SockAddr.cpp
	src/Buffer.cpp
	src/SockData.cpp
	res/agssock.rc
)
target_compile_definitions(agssock PRIVATE THIS_IS_THE_PLUGIN=1)
target_link_libraries(agssock PRIVATE $<$<PLATFORM_ID:Windows>:ws2_32>)

if(MSVC)
	# Compile runtime statically so that the dll does not require the redist kit
	target_compile_options(agssock PRIVATE /MT$<$<CONFIG:Debug>:d>)
endif()

if(MINGW OR MSYS)
	# Remove lib- prefix when compiling with MinGW
	set_target_properties(agssock PROPERTIES PREFIX "")
endif()

# Tests
enable_testing()

add_executable(test-buffer test/buffer.cpp test/Test.cpp)
add_test(Socket_buffer test-buffer)
