cmake_minimum_required(VERSION 3.22)
add_definitions(-std=c++17)
project(Dictionary LANGUAGES CXX)

set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

find_package(PkgConfig)
pkg_check_modules(gtkmm REQUIRED IMPORTED_TARGET gtkmm-4.0>=4.6)
find_package(Threads REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)
find_package(Boost 1.80 REQUIRED COMPONENTS system json)

add_executable(Dictionary
    src/main.cpp
    include/app.hpp
    include/dict.hpp
    include/json_body.hpp
)

target_link_libraries(Dictionary
    PRIVATE
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
    Boost::system Boost::json
    PkgConfig::gtkmm
)

target_include_directories(Dictionary
    PUBLIC
    include/
)
