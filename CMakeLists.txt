cmake_minimum_required(VERSION 3.10)
project(terminal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-fno-exceptions" COMPILER_SUPPORTS_FNO_EXCEPTIONS)
check_cxx_compiler_flag("-Wall" COMPILER_SUPPORTS_WALL)

if(COMPILER_SUPPORTS_FNO_EXCEPTIONS)
  add_compile_options(-fno-exceptions)
endif()
if(COMPILER_SUPPORTS_WALL)
  add_compile_options(-Wall)
endif()

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

add_executable(terminal
  "${SRC_DIR}/main.cpp"
  "${SRC_DIR}/PTY.cpp"
)
