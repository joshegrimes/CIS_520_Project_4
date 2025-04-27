cmake_minimum_required(VERSION 3.10)
project(mpi LANGUAGES C)

# Use C11 (change to 99 if you prefer)
set(CMAKE_C_STANDARD        11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Warnings + optimizations
add_compile_options(-Wall -O2)

# Find an MPI implementation
find_package(MPI REQUIRED)

# Declare the executable (you can rename “mpi” to whatever you like)
add_executable(mpi MPI.c)

# Link against MPI
target_link_libraries(mpi PRIVATE MPI::MPI_C)
