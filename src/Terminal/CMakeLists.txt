add_executable(terminal
  main.cpp
  PTY/PTY.cpp
)

target_link_libraries(terminal
  PRIVATE
    framework
)
