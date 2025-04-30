cmake_minimum_required(VERSION 3.27)
project(MagElementTestLinux)

set(CMAKE_CXX_STANDARD 20)
add_executable(MagElementTestLinux ../src/TcpConnectionTest.cpp ../src/TestOptions.cpp)

include_directories(. ../include ../../../boost_1_83_0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
TARGET_LINK_LIBRARIES(MagElementTestLinux Threads::Threads pthread)
