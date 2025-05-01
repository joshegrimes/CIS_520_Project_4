cmake_minimum_required(VERSION 3.10)
project(openmp C)

# Enable C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)

# Create executable
add_executable(openmp openmp.c)

target_compile_options(openmp PRIVATE -O2)

target_link_libraries(openmp PRIVATE OpenMP::OpenMP_C)

# Install
install(TARGETS openmp RUNTIME DESTINATION bin)
