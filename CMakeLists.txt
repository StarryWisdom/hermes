cmake_minimum_required(VERSION 3.15) # for CMP0091 / CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0066 NEW)
cmake_policy(SET CMP0091 NEW)

if (!${CMAKE_SIZEOF_VOID_P} EQUAL "8")
	message(FATAL_ERROR "I'm not building for 32 bit - I dont want to hunt bugs down for it")
endif ()

set(CMAKE_CXX_STANDARD 17) # it may be worth trying to bump this to C++20 sometimes

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

project(hermes CXX)

if (MSVC)
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)#only use conan on windows - it is probably not ideal using CONAN for one platform but it works currently
	conan_basic_setup(TARGETS)

	add_compile_definitions(_WIN32_WINNT=0x0601) #to suppress warnings, I think within the network-ts (was originally used for boost warnings, no longer needed)

	set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "" FORCE)

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL") #I would ideally link statically to libc for ease sending to AWS people, this currently doesnt work, failing in protobuf
	add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)#MSVC complaining about some C++ features that I depend on are going away (even though being called a warning it makes the build fail like an error), this includes the networking library which will be included in c++20
	add_compile_definitions(NOMINMAX)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT hermes)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	add_subdirectory(libs/googletest)#this probably should be shuffled into CONAN
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)#is this still needed? test both windows build + running test code
	set(PROTOBUF CONAN_PKG::protobuf)
	find_package(glm REQUIRED)
	set(GLM glm)
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wformat -Wformat-security -Wformat=2 -Wformat-nonliteral -ftrapv")
	if ("${CMAKE_BUILD_TYPE}" MATCHES Release)
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native") at some point this wants to come back but march native is kind of annoying
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 --coverage -fsanitize=undefined,address,leak")
		if (GCC)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=pointer-compare,pointer-subtract") # I am not 100% sure that this is needed in order to enable them in GCC, but it shouldnt do harm
		endif ()
	endif()
	#-rdaynmic was in here cant recall if that was obsolete or if it was for debugging somehow
	#-ldl used to be here but I dont remeber if its needed
	set(PROTOBUF protobuf)
	set(SYSTEM_LIBS pthread)
endif ()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Protobuf REQUIRED)

protobuf_generate_cpp(HERMES_PROTO_SRC HERMES_PROTO_HEADER src/hermes/hermes.proto)

set(LETO_CORE_SRC
	libs/lua/lapi.cpp
	libs/lua/lcode.cpp
	libs/lua/lctype.cpp
	libs/lua/ldebug.cpp
	libs/lua/ldo.cpp
	libs/lua/ldump.cpp
	libs/lua/lfunc.cpp
	libs/lua/lgc.cpp
	libs/lua/llex.cpp
	libs/lua/lmem.cpp
	libs/lua/lobject.cpp
	libs/lua/lopcodes.cpp
	libs/lua/lparser.cpp
	libs/lua/lstate.cpp
	libs/lua/lstring.cpp
	libs/lua/ltable.cpp
	libs/lua/ltm.cpp
	libs/lua/lundump.cpp
	libs/lua/lvm.cpp
	libs/lua/lzio.cpp
	libs/lua/lauxlib.cpp
	libs/lua/lbaselib.cpp
	libs/lua/lbitlib.cpp
	libs/lua/lcorolib.cpp
	libs/lua/ldblib.cpp
	libs/lua/liolib.cpp
	libs/lua/lmathlib.cpp
	libs/lua/loslib.cpp
	libs/lua/lstrlib.cpp
	libs/lua/ltablib.cpp
	libs/lua/lutf8lib.cpp
	libs/lua/loadlib.cpp
	libs/lua/linit.cpp

	src/core-lib/packet_buffer.cpp

	src/hermes/hermes.cpp
	src/hermes/settings.cpp
	src/dearimgui_link.cpp
	${HERMES_PROTO_SRC}
	${HERMES_PROTO_HDR}
)

add_executable (hermes.test
	${LETO_CORE_SRC}
	src/core-lib/packet_buffer.test.cpp
	src/core-lib/memory_buffer.test.cpp
	src/artemis-lib/bitstream.test.cpp
	src/artemis-lib/packet.test.cpp
	src/test.cpp

	src/hermes/hermes_proxy.test.cpp
)
target_link_libraries(hermes.test gtest gtest_main ${SYSTEM_LIBS} ${PROTOBUF} ${GLM} glfw ${OPENGL_gl_LIBRARY} ${OPENGL_LIBRARIES})
target_include_directories(hermes.test PUBLIC ${CMAKE_BINARY_DIR}/src/hermes ${CMAKE_BINARY_DIR} ${Protobuf_INCLUDE_DIRS} src libs/sol2/include libs/networking-ts-impl/include libs/lua libs/imgui libs/imgui/backends libs/crc32)

add_executable(hermes ${LETO_CORE_SRC} src/main.cpp)
target_link_libraries(hermes ${SYSTEM_LIBS} ${PROTOBUF} ${GLM} glfw ${OPENGL_gl_LIBRARY} ${OPENGL_LIBRARIES})
target_include_directories(hermes PUBLIC ${CMAKE_BINARY_DIR}/src/hermes ${CMAKE_BINARY_DIR} ${Protobuf_INCLUDE_DIRS} src libs/sol2/include libs/networking-ts-impl/include libs/lua libs/imgui libs/imgui/backends libs/crc32)
