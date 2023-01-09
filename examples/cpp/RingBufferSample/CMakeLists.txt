cmake_minimum_required(VERSION 3.21)

# Set the project name and version
project(RingBufferSample LANGUAGES CXX)
set(PROJECT_VERSION 0.1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
set(DLL_SOURCE_FILES
    ../../src/cpp/ConcurrentRingBuffer.h
    ../../src/cpp/ConcurrentRingBuffer.cpp
    ../../src/cpp/RingBuffer.h
    ../../src/cpp/RingBuffer.cpp
    ../../src/cpp/Span.h
)
set (SAMPLE_APP_SOURCE_FILES
    ConcurrentRingBufferSample.h
    RingBufferSample.h
    Main.cpp
)

# OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Targets
# Add a dynamic link library
add_library(SignalScatter SHARED ${DLL_SOURCE_FILES} ../../src/cpp/Api.cpp)
# Add a executable
add_executable(RingBufferSample ${DLL_SOURCE_FILES} ${SAMPLE_APP_SOURCE_FILES})