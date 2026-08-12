#include "PTY.h"

int main(int argc, char **argv) {
  if (PTY::getPTY()) {
    char shell[] = "/bin/zsh";
    PTY::getPTY().start_shell(shell);
  }
  return 0;
}
