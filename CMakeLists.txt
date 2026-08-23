cmake_minimum_required(VERSION 3.10)
project(terminal LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXCompilerFlag)
include(CheckOBJCXXCompilerFlag)

check_cxx_compiler_flag("-fno-exceptions" COMPILER_SUPPORTS_FNO_EXCEPTIONS)
check_cxx_compiler_flag("-Wall" COMPILER_SUPPORTS_WALL)
check_objcxx_compiler_flag("-fno-objc-arc" COMPILER_SUPPORTS_FNO_OBJC_ARC)

if(COMPILER_SUPPORTS_FNO_EXCEPTIONS)
  add_compile_options(-fno-exceptions)
endif()
if(COMPILER_SUPPORTS_WALL)
  add_compile_options(-Wall)
endif()
if(COMPILER_SUPPORTS_FNO_OBJC_ARC)
  add_compile_options("-fno-objc-arc")
endif()

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

add_executable(terminal
  "${SRC_DIR}/Framework/Application/CommonApplication.cpp"
  "${SRC_DIR}/Framework/Application/MacApplication.mm"
  "${SRC_DIR}/main.cpp"
  "${SRC_DIR}/PTY/PTY.cpp"
  "${SRC_DIR}/Framework/Window/MacWindow.mm"
  "${SRC_DIR}/Framework/Window/CommonWindow.cpp"
)

set(INCLUDE_DIR "${SRC_DIR}/Framework/include")

target_include_directories(terminal PRIVATE "${INCLUDE_DIR}")
target_link_libraries(terminal "-lpthread -framework Cocoa")
