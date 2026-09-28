#pragma once
#include "../include/IPTY.hpp"
#include <pthread.h>
#include <unistd.h>

// ----------------------------
// 具象クラス宣言
// ----------------------------

class PTY : public IPTY {
private:
  int ref_count = 0;

  IApplication *appInstance = nullptr;

  int master_fd = -1;
  char slave_path[256] = {0};

  // 読み込みスレッドを終了させるためのパイプ
  // 0: 読み込み, 1: 書き込み
  int read_thread_pipe[2] = {-1, -1};
  pthread_t read_thread = {};
  bool is_running = false;

  pid_t shell_pid = 0;

  void startShell(const char *) override;
  void writeInput(const void *, size_t) override;
  int release() override;
  int addRef() override;

  void destrustor_helper();
  void readLoop();
  PTY() { this->addRef(); };
  ~PTY();

  static void *readThreadEntry(void *);
  static void normalExitMsgHelper(int);

public:
  static PTY *createPTY(IApplication *);
};
