cmake_minimum_required(VERSION 3.20)
project(api-cpp-crow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(${PROJECT_NAME}-lib
	src/pq_conn_pool.cpp
)

target_include_directories(${PROJECT_NAME}-lib
    PUBLIC Crow/include
    PUBLIC include
)

find_package(fmt     CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(stduuid CONFIG REQUIRED)
find_library(LIBUUID_LIBRARY libuuid.a)

target_link_libraries(${PROJECT_NAME}-lib
    PUBLIC fmt::fmt
    PUBLIC libpqxx::pqxx
    PUBLIC stduuid
    PUBLIC ${LIBUUID_LIBRARY}
)

add_executable(${PROJECT_NAME}-exe
        src/main.cpp
)
target_link_libraries(${PROJECT_NAME}-exe ${PROJECT_NAME}-lib)
add_dependencies(${PROJECT_NAME}-exe ${PROJECT_NAME}-lib)
