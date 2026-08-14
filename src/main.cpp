#include "PTY.h"

int main(int argc, char **argv) {
  // 将来タブ機能をつけるために複数の PTY を作れるようにする
  // つまり GUI 化した時に接続する PTY を指定する仕組みが必要
  PTY *pty0 = PTY::createPTY();
  if (pty0) {
    char shell[] = "/bin/zsh";
    pty0->start_shell(shell);
    pty0->release();
  }
  return 0;
}
