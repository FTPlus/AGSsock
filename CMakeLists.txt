cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(CheckSymbolExists)
include(CheckTypeSize)

project("AGS Sockets" VERSION 0.1.0.0
	DESCRIPTION "Sockets plugin for Adventure Game Studio")
set(PROJECT_VERSION_TAG "beta 1")
#set(PROJECT_VERSION_TAG "${PROJECT_VERSION}")
set(PROJECT_AUTHOR "Ferry \\x22Wyz\\x22 Timmers")
set(PROJECT_AUTHORS "${PROJECT_AUTHOR}")
#set(PROJECT_AUTHORS "${PROJECT_AUTHOR} and ...")
set(PROJECT)
set(PROJECT_RELEASE_YEAR 2019)
set(PROJECT_RELEASE_MONTH 2)

configure_file(res/version.h.in res/version.h)

add_library(agssock MODULE
	src/agsplugin.cpp
	src/Socket.cpp
	src/API.cpp
	src/SockAddr.cpp
	src/Buffer.cpp
	src/SockData.cpp
	src/Pool.cpp
	res/agssock.rc
)
target_include_directories(agssock PUBLIC ${CMAKE_BINARY_DIR}/res)
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

if(NOT WIN32)
	check_symbol_exists("pthread_timedjoin_np" "pthread.h" HAVE_TIMEDJOIN)
	if(HAVE_TIMEDJOIN)
		add_definitions(-DHAVE_TIMEDJOIN)
	endif()
endif()

check_type_size("intptr_t" INTPTR_T)
if(HAVE_INTPTR_T)
	add_definitions(-DHAVE_INTPTR_T)
	if (NOT INTPTR_T EQUAL 4)
		message("WARNING Target platform is probably not 32 bits; this may cause problems with aligment of the plugin functions. If the plugin crashes when used, check if building for 32 bits averts the problem.")
	endif()
endif()

# Tests
enable_testing()

add_executable(test-buffer test/buffer.cpp test/Test.cpp)
add_test(Socket_buffer test-buffer)
