add_executable(terminal
  main.cpp
  PTY/PTY.cpp
  FontAtlas/FontAtlas.cpp
  Terminal/Terminal.cpp
)

target_link_libraries(terminal
  PRIVATE
    framework
)
