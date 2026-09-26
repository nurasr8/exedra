cmake_minimum_required(VERSION 3.16)
project(venim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall -Wextra)

find_package(CURL REQUIRED)

add_library(venimcore STATIC
    vnb.cpp
    db.cpp
    util.cpp
    adapter.cpp
    core.cpp
)
target_include_directories(venimcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(venimcore PUBLIC sqlite3 ssl crypto CURL::libcurl)

add_executable(venim main.cpp)
target_link_libraries(venim PRIVATE venimcore)

enable_testing()
add_executable(venim_test ../tests-cpp/test_venim.cpp)
target_link_libraries(venim_test PRIVATE venimcore)
add_test(NAME venim_unit COMMAND venim_test)
