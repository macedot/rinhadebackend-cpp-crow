cmake_minimum_required(VERSION 3.1)

set(project_name cpp_crown) ## rename your project here

project(${project_name})

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(${project_name}-lib
        src/pq_conn_pool.cpp
)

target_include_directories(${project_name}-lib PUBLIC src)

## link libs

find_package(Crow    REQUIRED)
find_package(fmt     REQUIRED)
find_package(libpqxx REQUIRED)

target_link_libraries(${project_name}-lib
        PUBLIC Crow::Crow
        PUBLIC libpqxx::pqxx
        PUBLIC fmt::fmt
)

#################################################################
## add executables

add_executable(${project_name}-exe
        src/cpp_crown.cpp
)
target_link_libraries(${project_name}-exe ${project_name}-lib)
add_dependencies(${project_name}-exe ${project_name}-lib)
