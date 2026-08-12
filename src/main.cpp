#include "PTY.h"

int main(int argc, char **argv) {
  if (PTY::getPTY()) {
    char cmd_line[] = "/bin/zsh";
    PTY::getPTY().start_shell(cmd_line);
  }
  return 0;
}
