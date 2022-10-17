include(CTest)

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set(CMAKE_C_FLAGS "-std=c99")

# set test build target name
set(TEST_BINARY mytests_app)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# add googletest subdirectory
add_subdirectory(googletest)

# this is so we can #include <gtest/gtest.h>
include_directories(../src)


include_directories(${PROJECT_SOURCE_DIR}/tests/googletest/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/mqtt)
include_directories(${PROJECT_SOURCE_DIR}/src/dev)
include_directories(${PROJECT_SOURCE_DIR}/src/lib)
include_directories(${PROJECT_SOURCE_DIR}/src/includes/configor)
include_directories(${PROJECT_SOURCE_DIR}/src/includes/asio2)
include_directories(${PROJECT_SOURCE_DIR}/src/data)
include_directories(${PROJECT_SOURCE_DIR}/src/data/data)
include_directories(${PROJECT_SOURCE_DIR}/src/data/setting)
include_directories(${PROJECT_SOURCE_DIR}/src/dev)
include_directories(${PROJECT_SOURCE_DIR}/src/app)
include_directories(${PROJECT_SOURCE_DIR}/src/app/dataprocess)
include_directories(${PROJECT_SOURCE_DIR}/src/app/up)
include_directories(${PROJECT_SOURCE_DIR}/src/app/up/report)
include_directories(${PROJECT_SOURCE_DIR}/src/app/up/center)
include_directories(${PROJECT_SOURCE_DIR}/src/app/up/packet)
include_directories(${PROJECT_SOURCE_DIR}/src/app/down)
include_directories(${PROJECT_SOURCE_DIR}/src/test)
include_directories(${PROJECT_SOURCE_DIR}/src/data/easysqlite/includes)
include_directories(${PROJECT_SOURCE_DIR}/src/data/easysqlite)
include_directories(${PROJECT_SOURCE_DIR}/src/boost)
include_directories(${PROJECT_SOURCE_DIR}/src/boost/pfr)
include_directories(${PROJECT_SOURCE_DIR}/src/includes/mqttclient)
include_directories(${PROJECT_SOURCE_DIR}/src/includes/mqttclient/mbedtls)

add_test(test_1 ${TEST_BINARY})

# add new test
aux_source_directory(${PROJECT_SOURCE_DIR}/tests SRC_MAIN)
add_executable(${TEST_BINARY} ${SRC_MAIN})
TARGET_LINK_LIBRARIES(${TEST_BINARY} -lzlog -leasysqlite ) #-lmqttclient)
target_link_libraries(${TEST_BINARY} Threads::Threads)
target_link_libraries(${TEST_BINARY} gtest gtest_main app-cjq-lib)