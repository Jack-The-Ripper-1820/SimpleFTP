# CMakeList.txt : CMake project for FTPThread, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("FTPThread")

# Add source to this project's executable.
add_executable (server "server.cpp")
add_executable (client "client.cpp")

set(CMAKE_TOOLCHAIN_FILE "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "C:/src/vcpkg/installed/x64-windows/share")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET server PROPERTY CXX_STANDARD 20)
  set_property(TARGET client PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

if(WIN32)
  target_link_libraries(server wsock32 ws2_32)
  target_link_libraries(client wsock32 ws2_32)
endif()