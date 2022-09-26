cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 14)

#版本迭代编号+编译时间.  
set(VERSION_NUMBER  V1.1.14) 
string(TIMESTAMP COMPILE_TIME ${VERSION_NUMBER}_%y%m%d_%H.%M.%S) 
set(build_time    ${COMPILE_TIME})

#需要在源码目录下创建VersionConfig.h.in和VersionConfig.h
configure_file( 
    "VersionConfig.h.in"
    "VersionConfig.h"
)

# Set the project name
# project (app-cjq_${build_time})
project (app-cjq)
#set (target  AAA_${build_time})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "-lrt")
set(CMAKE_C_FLAGS         "${CMAKE_CXX_FLAGS_DEBUG} -fpermissive -Wpointer-arith  -Wno-psabi -Wall -pedantic -std=c99")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fpermissive -Wpointer-arith  -Wno-psabi -Wall -pedantic -std=c99")
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}   -fpermissive -Wpointer-arith  -Wno-psabi -Wall -pedantic -std=c99")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++14 ${CMAKE_CXX_FLAGS_RELEASE} -fpermissive -Wpointer-arith  -Wno-psabi")
set(CMAKE_CXX_FLAGS_DEBUG   "-std=c++14 ${CMAKE_CXX_FLAGS_DEBUG} -fpermissive -Wpointer-arith  -Wno-psabi")
set(CMAKE_CXX_FLAGS         "-std=c++14   ${CMAKE_CXX_FLAGS_RELEASE} -fpermissive -Wpointer-arith")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/data)
include_directories(${PROJECT_SOURCE_DIR}/data/data)
include_directories(${PROJECT_SOURCE_DIR}/data/setting)
include_directories(${PROJECT_SOURCE_DIR}/dev)
include_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(${PROJECT_SOURCE_DIR}/lib/cjson)
include_directories(${PROJECT_SOURCE_DIR}/dev)
include_directories(${PROJECT_SOURCE_DIR}/lib/configor)
include_directories(${PROJECT_SOURCE_DIR}/app)
include_directories(${PROJECT_SOURCE_DIR}/app/dataprocess)
include_directories(${PROJECT_SOURCE_DIR}/app/up)
include_directories(${PROJECT_SOURCE_DIR}/app/up/report)
include_directories(${PROJECT_SOURCE_DIR}/app/up/center)
include_directories(${PROJECT_SOURCE_DIR}/app/up/packet)
include_directories(${PROJECT_SOURCE_DIR}/app/down)
include_directories(${PROJECT_SOURCE_DIR}/test)
include_directories(${PROJECT_SOURCE_DIR}/data/easysqlite/includes)
include_directories(${PROJECT_SOURCE_DIR}/boost)
include_directories(${PROJECT_SOURCE_DIR}/boost/pfr)
include_directories(${PROJECT_SOURCE_DIR}/includes/mqttclient)
include_directories(${PROJECT_SOURCE_DIR}/includes/mqttclient/mbedtls)

#向工程添加多个特定的库文件搜索路径
# link_directories(
# 	include
# 	${OpenCV_LIBRARY_DIRS}
# 	${PCL_LIBRARY_DIRS}
# 	${GTSAM_LIBRARY_DIRS}
# )

# Add a library with the above sources
aux_source_directory(./app/ SRC_MAIN)
aux_source_directory(./app/dataprocess/ SRC_MAIN)
aux_source_directory(./app/down/ SRC_MAIN)
aux_source_directory(./app/up/ SRC_MAIN)
aux_source_directory(./app/up/packet/ SRC_MAIN)
aux_source_directory(./app/up/report/ SRC_MAIN)
aux_source_directory(./app/up/center/ SRC_MAIN)
aux_source_directory(./data/ SRC_MAIN)
aux_source_directory(./data/data/ SRC_MAIN)
aux_source_directory(./data/setting/ SRC_MAIN)
aux_source_directory(./dev/ SRC_MAIN)
aux_source_directory(./lib/ SRC_MAIN)
aux_source_directory(./lib/configor/ SRC_MAIN)
aux_source_directory(./data/easysqlite/ SRC_MAIN)

add_executable(${PROJECT_NAME} ${SRC_MAIN})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lzlog -leasysqlite -lmqttclient)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_include_directories( ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

if ( BUILDUSING_GTEST STREQUAL OFF)
    message("生产模式")
else()
    message("调试模式.链接生成gtest库")
    add_library(app-cjq-lib  ${SRC_MAIN})
endif()