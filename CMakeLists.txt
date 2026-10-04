cmake_minimum_required(VERSION 3.16)
project(terminal LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
  message(STATUS
    "AddressSanitizer is ON (Use-After-Free / Double-Free check mode)")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

include(CheckCXXCompilerFlag)
include(CheckOBJCXXCompilerFlag)

check_cxx_compiler_flag("-fno-exceptions" COMPILER_SUPPORTS_FNO_EXCEPTIONS)
check_cxx_compiler_flag("-Wall" COMPILER_SUPPORTS_WALL)
check_cxx_compiler_flag("-fno-rtti" COMPILER_SUPPORTS_FNO_RTTI)
check_objcxx_compiler_flag("-fno-objc-arc" COMPILER_SUPPORTS_FNO_OBJC_ARC)

if(COMPILER_SUPPORTS_FNO_EXCEPTIONS)
  add_compile_options(-fno-exceptions)
endif()
if(COMPILER_SUPPORTS_WALL)
  add_compile_options(-Wall)
endif()
if(COMPILER_SUPPORTS_FNO_RTTI)
  add_compile_options(-fno-rtti)
endif()
if(COMPILER_SUPPORTS_FNO_OBJC_ARC)
  add_compile_options("-fno-objc-arc")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}"
)

add_subdirectory(src/Framework)

add_subdirectory(src/Terminal)
