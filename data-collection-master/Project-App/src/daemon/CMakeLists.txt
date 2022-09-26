cmake_minimum_required(VERSION 3.10)

#版本迭代编号+编译时间.  
set(VERSION_NUMBER  V1.1.05) 
string(TIMESTAMP COMPILE_TIME ${VERSION_NUMBER}_%y%m%d_%H.%M.%S) 
set(build_time    ${COMPILE_TIME})

#需要在源码目录下创建VersionConfig.h.in和VersionConfig.h
configure_file( 
    "VersionConfig.h.in"
    "VersionConfig.h"
)

# Set the project name
#project (app-daemon_${build_time})
project (app-daemon)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "-lrt")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fpermissive -Wpointer-arith   -Wno-psabi")
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -fpermissive -Wpointer-arith   -Wno-psabi")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fpermissive -Wpointer-arith   -Wno-psabi")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fpermissive -Wpointer-arith   -Wno-psabi")
set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS_RELEASE} -fpermissive -Wpointer-arith   -Wno-psabi")
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS_DEBUG} -fpermissive -Wpointer-arith   -Wno-psabi")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/configor)

# Add a library with the above sources

aux_source_directory(./configor/ SRC_MAIN)
add_executable(${PROJECT_NAME} 
        daemon.cpp 
        ftpclient.cpp 
        lib.cpp 
        updata.cpp
        ${SRC_MAIN})

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_include_directories( ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})


