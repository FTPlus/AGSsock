cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(CheckLibraryExists)
include(CheckTypeSize)

# [Version] Set-up plugin meta data
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

# [Core] Set-up the core elements of the plugin
add_library(agssock-core STATIC
	src/Socket.cpp
	src/API.cpp
	src/SockAddr.cpp
	src/Buffer.cpp
	src/SockData.cpp
	src/Pool.cpp
)
target_compile_definitions(agssock-core PUBLIC THIS_IS_THE_PLUGIN=1)
target_include_directories(agssock-core PUBLIC ${CMAKE_BINARY_DIR}/res)
target_link_libraries(agssock-core PUBLIC
	$<$<PLATFORM_ID:Windows>:ws2_32>
	$<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)
target_compile_features(agssock-core PUBLIC cxx_deleted_functions)
# ... and more, I can't be bothered to name them all
set_target_properties(agssock-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Platform detection used by the AGS plugin interface
# Probably currently incomplete
if(WIN32)
	target_compile_definitions(agssock-core PUBLIC WINDOWS_VERSION)
elseif(APPLE)
	target_compile_definitions(agssock-core PUBLIC MAC_VERSION)
elseif(UNIX)
	target_compile_definitions(agssock-core PUBLIC LINUX_VERSION)
endif()

# [Plugin] Set-up the plugin itself
add_library(agssock MODULE src/agsplugin.cpp res/agssock.rc)
target_link_libraries(agssock PRIVATE agssock-core)

# Platform specific set-up
if(MSVC)
	# Compile runtime statically so that the dll does not require the redist kit
	target_compile_options(agssock-core PUBLIC /MT$<$<CONFIG:Debug>:d> /wd4996)
endif()

if(MINGW OR MSYS)
	# Remove lib- prefix when compiling with MinGW
	set_target_properties(agssock PROPERTIES PREFIX "")
endif()

if(NOT WIN32)
	check_library_exists(pthread pthread_timedjoin_np "" HAVE_TIMEDJOIN)
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

# [Tests]
enable_testing()

add_executable(test-buffer test/buffer.cpp test/Test.cpp)
target_include_directories(test-buffer PRIVATE src)
target_link_libraries(test-buffer PRIVATE agssock-core)
target_compile_features(test-buffer PRIVATE cxx_generic_lambdas)
add_test(Socket_buffer test-buffer)

add_executable(test-pool test/pool.cpp test/Test.cpp)
target_include_directories(test-pool PRIVATE src)
target_link_libraries(test-pool PRIVATE agssock-core)
target_compile_features(test-pool PRIVATE cxx_generic_lambdas)
add_test(Socket_pool test-pool)
