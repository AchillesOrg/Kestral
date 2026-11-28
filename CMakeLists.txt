cmake_minimum_required(VERSION 3.16)
project(kestral VERSION 2.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /utf-8 /EHsc)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(kestral_core STATIC
  src/lexer.cpp
  src/parser.cpp
  src/interp.cpp
  src/builtins.cpp
  src/nana.cpp
  src/learn.cpp
  src/repl.cpp
)
target_include_directories(kestral_core PUBLIC src)

add_executable(kestral src/main.cpp)
target_link_libraries(kestral PRIVATE kestral_core)

add_executable(test_kestral tests/test_kestral.cpp)
target_link_libraries(test_kestral PRIVATE kestral_core)

if(MSVC)
  target_link_options(kestral PRIVATE /STACK:8388608)
  target_link_options(test_kestral PRIVATE /STACK:8388608)
endif()

enable_testing()
add_test(NAME kestral_tests COMMAND test_kestral WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
