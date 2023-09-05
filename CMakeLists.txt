cmake_minimum_required(VERSION 3.20)
set(project_name api-cpp-crow) ## rename your project here
project(${project_name} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(${project_name}-lib
	src/pq_conn_pool.cpp
)

target_include_directories(${project_name}-lib
    PUBLIC include
)

find_package(Crow    CONFIG REQUIRED)
find_package(fmt     CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

target_link_libraries(${project_name}-lib
    PUBLIC Crow::Crow
    PUBLIC libpqxx::pqxx
    PUBLIC fmt::fmt
)

add_executable(${project_name}-exe
        src/App.cpp
)
target_link_libraries(${project_name}-exe ${project_name}-lib)
add_dependencies(${project_name}-exe ${project_name}-lib)
