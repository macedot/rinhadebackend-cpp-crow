include(FetchContent)
cmake_minimum_required(VERSION 3.20)
set(PROJECT_NAME api-cpp-crow)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(${PROJECT_NAME}-lib
	src/pq_conn_pool.cpp
)

target_include_directories(${PROJECT_NAME}-lib
    PUBLIC include
)

find_path(LIBPQXX_INCLUDE_DIR pqxx)
FetchContent_Declare(
  libpqxx
  GIT_REPOSITORY https://github.com/jtv/libpqxx.git
  GIT_TAG        7.8.1
  GIT_PROGRESS   TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)
set(SKIP_PQXX_SHARED ON)
set(SKIP_BUILD_TEST ON)
set(PQXX_LIBRARIES pqxx_static)
FetchContent_MakeAvailable(libpqxx)
FetchContent_GetProperties(libpqxx)
set(HAVE_LIBPQXX7 TRUE)
target_include_directories(${PROJECT_NAME}-lib  PRIVATE ${libpqxx_SOURCE_DIR}/include)

#set(FMT_TEST FF CACHE INTERNAL "disabling fmt tests")
FetchContent_Declare(
        fmt
        GIT_REPOSITORY  https://github.com/fmtlib/fmt.git
        GIT_TAG         10.1.1
        GIT_PROGRESS    TRUE
        USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(fmt)

find_package(Crow CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}-lib fmt::fmt-header-only Crow::Crow pqxx)

add_executable(${PROJECT_NAME}-exe
        src/App.cpp
)
target_link_libraries(${PROJECT_NAME}-exe ${PROJECT_NAME}-lib)
add_dependencies(${PROJECT_NAME}-exe ${PROJECT_NAME}-lib)
