cmake_minimum_required(VERSION 3.21)

# Set the project name and version
project(BitonicSortSample LANGUAGES CXX)
set(PROJECT_VERSION 0.1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
set(DLL_SOURCE_FILES
    ../../../src/cuda/BitonicSort.cu
    ../../../src/cuda/BitonicSort.h
)
set (SAMPLE_APP_SOURCE_FILES
    Main.cpp
    BitonicSortSample.cu
    BitonicSortSample.h
)

# CUDA
find_package(CUDAToolkit REQUIRED)
if (CUDAToolkit_FOUND)
    enable_language(CUDA)
    include_directories(${CUDAToolkit_INCLUDE_DIRS})
endif()

# Targets
## Add a dynamic link library
add_library(bitonicsort SHARED ${DLL_SOURCE_FILES})
set_target_properties(bitonicsort PROPERTIES LINKER_LANGUAGE CXX)
## Add a executable
add_executable(${PROJECT_NAME} ${SAMPLE_APP_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} bitonicsort) 

# Logs
message(STATUS "CUDAToolkit_FOUND: ${CUDAToolkit_FOUND}")
message(STATUS "CUDAToolkit_INCLUDE_DIRS: ${CUDAToolkit_INCLUDE_DIRS}")
message(STATUS "CUDAToolkit_LIBRARY_DIR: ${CUDAToolkit_LIBRARY_DIR}")
# message(STATUS "CUDAToolkit_LIBRARIES: ${CUDAToolkit_LIBRARIES}")
# message(STATUS "SOURCES: ${SOURCES}")