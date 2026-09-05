cmake_minimum_required(VERSION 3.10)
project(terminal LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
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

# ----------------------------
# Shader
# ----------------------------
find_program(XCRUN xcrun REQUIRED)
find_program(XXD_TOOL xxd REQUIRED)

set(SHADER_SRC "${SRC_DIR}/Framework/Graphics/Shaders.metal")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SHADER_AIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders.air")
set(SHADER_LIB "${CMAKE_CURRENT_BINARY_DIR}/Shaders.metallib")
set(SHADER_HEADER "${GEN_DIR}/Shaders_metallib.h")

file(MAKE_DIRECTORY "${GEN_DIR}")

add_custom_command(
  OUTPUT "${SHADER_HEADER}"
  COMMAND "${XCRUN}" -sdk macosx metal -c "${SHADER_SRC}" -o "${SHADER_AIR}"
  COMMAND "${XCRUN}" -sdk macosx metallib "${SHADER_AIR}" -o "${SHADER_LIB}"
  # xxd -i で C 言語の unsigned char 配列ヘッダーに変換する
  COMMAND "${XXD_TOOL}" -i "Shaders.metallib" "${SHADER_HEADER}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${SHADER_SRC}"
  COMMENT "Compiling Metal Shader to Header: ${SHADER_HEADER}"
  VERBATIM
)

# ----------------------------
# Build
# ----------------------------
add_executable(terminal
  "${SRC_DIR}/Framework/Application/CommonApplication.cpp"
  "${SRC_DIR}/Framework/Application/MacApplication.mm"
  "${SRC_DIR}/Framework/Window/MacWindow.mm"
  "${SRC_DIR}/Framework/Window/CommonWindow.cpp"
  "${SRC_DIR}/Framework/Graphics/MacGraphics.mm"
  "${SRC_DIR}/Framework/Graphics/CommonGraphics.cpp"
  "${SRC_DIR}/main.cpp"
  "${SRC_DIR}/PTY/PTY.cpp"
  "${SHADER_HEADER}"
)

set(INCLUDE_DIR "${SRC_DIR}/Framework/include")

target_include_directories(terminal PRIVATE
  "${INCLUDE_DIR}"
  "${GEN_DIR}"
)

target_link_libraries(terminal PRIVATE
  pthread
  "-framework Cocoa"
  "-framework Metal"
  "-framework QuartzCore"
)
