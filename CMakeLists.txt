cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(CheckLibraryExists)
include(CheckTypeSize)

# [Platform detection] used by the AGS plugin interface
# Probably currently incomplete
if(WIN32)
	set(AGS_VERSION WINDOWS_VERSION)
elseif(APPLE)
	set(AGS_VERSION MAC_VERSION)
elseif(UNIX)
	set(AGS_VERSION LINUX_VERSION)
endif()

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
set(PROJECT_RELEASE_MONTH 3)

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
target_compile_definitions(agssock-core PUBLIC THIS_IS_THE_PLUGIN=1 ${AGS_VERSION})
target_include_directories(agssock-core PUBLIC ${CMAKE_BINARY_DIR}/res)
target_link_libraries(agssock-core PUBLIC
	$<$<PLATFORM_ID:Windows>:ws2_32>
	$<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)
target_compile_features(agssock-core PUBLIC cxx_deleted_functions)
# ... and more, I can't be bothered to name them all
set_target_properties(agssock-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
endif()
check_type_size("int" INT)
if (NOT INT EQUAL 4)
	message(WARNING "AGS makes the (unfortunate) assumption that the size of 'int' is 4 bytes. The target platform has 'int' of size ${INT}; this may cause memory alignment problems when using the plug-in. If the plug-in crashes or does not work as intended, check if building for 32 bits averts the problem.")
endif()
if (NOT INTPTR_T EQUAL INT)
	message(WARNING "AGS makes the (unfortunate) assumption that the size of 'int' is large enough to store pointer values. The target platform supports pointers larger than this; this may cause pointer addresses to be sliced when using the plug-in. If the plug-in crashes or does not work as intended, check if building for 32 bits averts the problem.")
endif()

# [Test interface]
enable_testing()

add_library(tester test/tester/Test.cpp)
target_include_directories(tester PUBLIC test/tester)
target_compile_features(tester PUBLIC cxx_generic_lambdas)

add_library(agsmock
	test/agsmock/agsmock.cpp
	test/agsmock/engine.cpp
	test/agsmock/Library.cpp
)
target_include_directories(agsmock PRIVATE src test/agsmock)
target_link_libraries(agsmock PRIVATE $<$<NOT:$<PLATFORM_ID:Windows>>:dl>)
target_compile_definitions(agsmock PUBLIC ${AGS_VERSION})
target_compile_features(agsmock PUBLIC cxx_nullptr)

if(MSVC)
	target_compile_options(tester PUBLIC /MT$<$<CONFIG:Debug>:d> /wd4996)
	target_compile_options(agsmock PUBLIC /MT$<$<CONFIG:Debug>:d> /wd4996)
endif()

# [Tests]
add_executable(test-buffer test/buffer.cpp)
target_include_directories(test-buffer PRIVATE src)
target_link_libraries(test-buffer PRIVATE tester agssock-core)
add_test(Socket_buffer test-buffer)

add_executable(test-pool test/pool.cpp)
target_include_directories(test-pool PRIVATE src)
target_link_libraries(test-pool PRIVATE tester agssock-core)
add_test(Socket_pool test-pool)

add_executable(test-sockaddr test/sockaddr.cpp)
target_link_libraries(test-sockaddr PRIVATE tester agsmock)
add_test(SockAddr test-sockaddr)

add_executable(test-socket test/socket.cpp)
target_link_libraries(test-socket PRIVATE tester agsmock)
add_test(Socket test-socket)

