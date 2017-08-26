cmake_minimum_required (VERSION 2.8.2)

project(AGSsock)

if (WIN32)
	# Compile runtime statically so that the dll does not require the redist kit
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif ()

add_definitions(-DTHIS_IS_THE_PLUGIN=1)

add_library(agssock SHARED src/agsplugin.cpp src/Socket.cpp src/API.cpp src/SockAddr.cpp src/Buffer.cpp src/SockData.cpp res/agssock.rc)

if (WIN32)
	target_link_libraries(agssock ws2_32)
	set_target_properties(agssock PROPERTIES PREFIX "")
endif ()