# set project name and version
project(gtestTest)

# cmake version require
cmake_minimum_required(VERSION 3.0.0)

# file to compile
add_subdirectory(src) 

# for gtest, included in tests sub-folder
enable_testing()
add_subdirectory(tests)
# assign cpp standard
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

set(CMAKE_CXX_COMPILER /home/kaihua/.bin/g++)
set(CMAKE_C_COMPILER /home/kaihua/.bin/gcc)

set(CMAKE_CXX_FLAGS "-std=c++17")
set(CMAKE_C_FLAGS "-std=c99")
add_definitions(-D_GLIBCXX_USE_C99=1)

# add include and src path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)




