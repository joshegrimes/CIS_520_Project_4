cmake_minimum_required(VERSION 3.10)
project(3way-pthread LANGUAGES C)

# Use C11 (or change to 99 if you prefer)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings and optimizations
add_compile_options(-Wall -O2)

# Find the Threads package (for pthreads)
find_package(Threads REQUIRED)

# Declare the executable
add_executable(pthread pthread.c)

# Link in the pthread library
target_link_libraries(pthread PRIVATE Threads::Threads)
