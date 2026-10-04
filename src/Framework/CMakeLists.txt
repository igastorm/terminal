find_program(XCRUN xcrun REQUIRED)

set(SHADER_SRC
      "${CMAKE_SOURCE_DIR}/src/Framework/Graphics/Mac/MacShaders.metal")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SHADER_AIR "${CMAKE_CURRENT_BINARY_DIR}/shaders.air")
set(SHADER_LIB "${CMAKE_CURRENT_BINARY_DIR}/shaders.metallib")
set(SHADER_HEADER "${GEN_DIR}/shaders_metallib.hpp")
set(BIN2H_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/bin2hpp.cmake")

file(MAKE_DIRECTORY "${GEN_DIR}")

add_custom_command(
  OUTPUT "${SHADER_HEADER}"
  BYPRODUCTS "${SHADER_AIR}" "${SHADER_LIB}"
  COMMAND "${XCRUN}" -sdk macosx metal -c "${SHADER_SRC}" -o "${SHADER_AIR}"
  COMMAND "${XCRUN}" -sdk macosx metallib "${SHADER_AIR}" -o "${SHADER_LIB}"
  COMMAND "${CMAKE_COMMAND}"
            "-DINPUT=${SHADER_LIB}"
            "-DOUTPUT=${SHADER_HEADER}"
            "-DVARNAME=shaders_metallib"
            -P "${BIN2H_SCRIPT}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${SHADER_SRC}"
  COMMENT "Compiling Metal Shader to Header: ${SHADER_HEADER}"
  VERBATIM
)

add_library(framework STATIC
  "Application/CommonApplication.cpp"
  "Application/Mac/MacApplication.mm"
  "Window/Mac/MacWindow.mm"
  "Window/CommonWindow.cpp"
  "Graphics/Mac/MacGraphicsDevice.mm"
  "Graphics/Mac/MacSurface.mm"
  "Graphics/Mac/MacWindowSurface.mm"
  "Graphics/Mac/MacTextureSurface.mm"
  "Graphics/Mac/MacTexture.mm"
  "Graphics/Mac/MacRenderPass.mm"
  "Graphics/Mac/MacFontAtlas.cpp"
  "Graphics/Mac/MacBitmap.cpp"
  "Graphics/Mac/MacFont.cpp"
  "Graphics/CommonGraphics.cpp"
  "CharConverter/CharConverter.cpp"
  "${SHADER_HEADER}"
)

set(FRAMEWORK_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(framework
  PUBLIC
    "${FRAMEWORK_INCLUDE_DIR}"

  PRIVATE
    "${GEN_DIR}"
)

target_link_libraries(framework
  PRIVATE
    pthread
    "-framework Cocoa"
    "-framework Metal"
    "-framework QuartzCore"
)
