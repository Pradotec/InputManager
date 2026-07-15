cmake_minimum_required(VERSION 3.16)
project(InputManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INPUT_MANAGER_BUILD_EXAMPLES "Build example programs" ON)
option(INPUT_MANAGER_BUILD_TESTS "Build tests" ON)

if(WIN32)
    set(PLATFORM_LIBS ws2_32 setupapi)
elseif(UNIX)
    set(PLATFORM_LIBS pthread)
endif()

add_library(input_manager
    src/transport/serial_port.cpp
    src/transport/udp_client.cpp
    src/devices/device_base.cpp
    src/devices/kmbox_b.cpp
    src/devices/kmbox_net.cpp
    src/devices/makcu.cpp
    src/input_manager.cpp
)

target_include_directories(input_manager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(input_manager PRIVATE ${PLATFORM_LIBS})

if(INPUT_MANAGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(INPUT_MANAGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS input_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
