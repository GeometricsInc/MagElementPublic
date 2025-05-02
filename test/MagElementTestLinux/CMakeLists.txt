cmake_minimum_required(VERSION 3.27)
project(MagElementTestLinux)

set(CMAKE_CXX_STANDARD 17)
add_executable(MagElementTestLinux ../src/TestClient.cpp ../src/TestOptions.cpp)

#target_compile_features(TestClient.o PROPERTIES cxx_std_17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 --verbose")

include_directories(. ../include ../../../boost_1_83_0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
TARGET_LINK_LIBRARIES(MagElementTestLinux Threads::Threads pthread)
