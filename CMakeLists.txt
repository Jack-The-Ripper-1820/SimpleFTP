# CMakeList.txt : CMake project for FTPThread, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("FTPThread")

# Add source to this project's executable.
add_executable (Server "Server.cpp")
add_executable (Client "Client.cpp")

set(CMAKE_TOOLCHAIN_FILE "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "C:/src/vcpkg/installed/x64-windows/share")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Server PROPERTY CXX_STANDARD 20)
  set_property(TARGET Client PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

if(WIN32)
  target_link_libraries(Server wsock32 ws2_32)
  target_link_libraries(Client wsock32 ws2_32)
endif()