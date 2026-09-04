#include "IPTY.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <new> // IWYU pragma: keep
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>

namespace {
// ----------------------------
// 具象クラス宣言
// ----------------------------

class ImpPTY : public IPTY {
private:
  int ref_count = 0;

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

  void close();
  void read_loop();
  ImpPTY() = default;
  ~ImpPTY();

  static void *read_thread_entry(void *);
  static void normalExitMsgHelper(int);

public:
  static ImpPTY *createPTY();
};

// ----------------------------
// メンバ関数の実装
// ----------------------------

int ImpPTY::addRef() { return ++this->ref_count; }

int ImpPTY::release() {
  if (--this->ref_count == 0) {
    this->~ImpPTY();
    free(this);
    return 0;
  }
  return this->ref_count;
}

void ImpPTY::normalExitMsgHelper(int status) {
  int exit_code = WEXITSTATUS(status);
  std::cout << "\n[INFO] Shell exited normally with code: " << exit_code
            << std::endl;
}

ImpPTY::~ImpPTY() { this->close(); }

void ImpPTY::close() {
  if (this->read_thread_pipe[1] >= 0) {
    const char c = 'q';
    // シェルの終了通知
    // すでにシェルが終了していればこれは空振りするだけ
    // (ループを抜けているはず)
    write(this->read_thread_pipe[1], &c, sizeof(c));

    // 読み込みスレッドが生成される前にエラー等で終了するときに join
    // するとまずい
    if (this->is_running) {
      this->is_running = false;
      pthread_join(this->read_thread,
                   nullptr); // 読み取りスレッドの終了を待つ
      std::cout << "\n[INFO] PTY Closed.\n";
    }

    if (this->read_thread_pipe[0] >= 0) {
      ::close(this->read_thread_pipe[0]);
      this->read_thread_pipe[0] = -1;
    }
    if (this->read_thread_pipe[1] >= 0) {
      ::close(this->read_thread_pipe[1]);
      this->read_thread_pipe[1] = -1;
    }
  }
  // シェルの後始末
  if (this->shell_pid > 0) {
    int status = 0;
    if (waitpid(this->shell_pid, &status, WNOHANG) == 0) {
      kill(this->shell_pid, SIGKILL);
      // 即座に kill するからブロッキングしもいい
      waitpid(this->shell_pid, &status, 0);
    }
    if (WIFEXITED(status)) {
      normalExitMsgHelper(status);
    } else if (WIFSIGNALED(status)) {
      int sig = WTERMSIG(status);
      std::cout << "\n[INFO] Shell killed by signal: " << sig << std::endl;
    }
    this->shell_pid = 0;
  }
  if (master_fd >= 0) {
    ::close(master_fd);
    this->master_fd = -1;
    std::cout << std::endl << "Exit the terminal" << std::endl;
  } else {
    std::cout << std::endl
              << "The file descriptor does not exist." << std::endl;
  }
}

void ImpPTY::writeInput(const void *data, size_t len) {
  if (this->is_running) {
    if (this->master_fd >= 0 && data != nullptr && len > 0) {
      write(this->master_fd, data, len);
    }
  }
}

void *ImpPTY::read_thread_entry(void *args) {
  ImpPTY *pty = reinterpret_cast<ImpPTY *>(args);
  pty->read_loop();
  return nullptr;
}

void ImpPTY::read_loop() {
  // シェルの起動が失敗したら何かしらのメッセージかダイアログを出すべき
  // read
  // では読み取った分だけシークするので溢れたら自動的に複数に分割して読み込めるから
  // 1024 あればいいと思われる
  // ただしエスケープシーケンスの途中で切れる可能性も考慮する必要がある
  char buffer[1024];
  pollfd pfds[2] = {};
  pfds[0].fd = this->master_fd;
  pfds[0].events = POLLIN; // masterfd への入力 (つまりシェルの出力) を監視
  pfds[1].fd = this->read_thread_pipe[0];
  pfds[1].events = POLLIN;

  while (true) {
    int ret = poll(pfds, 2, -1);
    if (ret < 0 || (pfds[1].revents & POLLIN)) {
      if (this->shell_pid > 0) {
        kill(this->shell_pid, SIGHUP);
      }
      break;
    }

    if (pfds[0].revents & POLLIN) {
      ssize_t bytes_read = read(this->master_fd, buffer, sizeof(buffer));
      // シェルが終了すると slavefd が閉じられる
      // slavefd が閉じれれている時に msterfd を読み取ると read の戻りが
      // macOS では 0
      // Linux では -1 になるらしい
      if (bytes_read <= 0) {
        // シェルプロセスの後始末
        // なんでわざわざゾンビ状態というのがあるのかと思ったら終了コードを取得するためだった
        // つまり終了コードを受け取るコードがないといつまででも親切に待っていてくれてしまうということ
        int status = 0;
        // すでにシェルは終了しているのでブロッキングしても大丈夫 (すぐ返る)
        waitpid(this->shell_pid, &status, 0);
        if (WIFEXITED(status)) {
          normalExitMsgHelper(status);
        }
        this->shell_pid = 0;
        break;
      }
      write(STDOUT_FILENO, buffer, bytes_read);
    }
  }
}

void ImpPTY::startShell(const char *shell) {
  // ----------------------------
  // シェルを起動 (fork する)
  // ----------------------------
  // fork すると同じ内容のサブプロセスが作られる (実行位置は fork() の直後)
  pid_t pid = fork();
  if (pid < 0) {
    std::perror("fork failed");
    return;
  }

  // 子プロセスの場合は pid == 0 が返ってくる
  // つまり子プロセスのみで行いたい処理はここに書けば良い
  if (pid == 0) {
    // --- 子プロセス側の処理 (zsh になる予定のプロセス)---

    // デバッグ用環境変数を除去
    unsetenv("MallocStackLogging");
    unsetenv("MallocStackLoggingNoCompact");
    unsetenv("ASAN_OPTIONS");
    unsetenv("MTL_DEBUG_LAYER");

    // 親プロセスの Master fd は不要なので閉じる (参照カウンタを減らす)
    ::close(this->master_fd);

    // 同様に親プロセスのパイプは不要なので閉じる (参照カウンタを減らす)
    ::close(this->read_thread_pipe[0]);
    ::close(this->read_thread_pipe[1]);

    // 新しいセッションを作成し、プロセスグループのリーダーになる
    // 標準ターミナルとの縁を切って無理やり自作 pty
    // に配管を繋ぎかえるイメージらしい
    setsid();

    // Slave 側のファイル記述子を開く
    int slave_fd = open(this->slave_path, O_RDWR);
    if (slave_fd < 0) {
      std::perror("open slave failed");
      ::_exit(1);
    }

    // この Slave をプロセスの制御端末 (Controlling Terminal) に設定する
    // BSD 系 Unix では open で O_NOCTTY
    // を付けなくても自動的に制御端末にならないから必要らしい
    // 逆に SystemV 系だと自動的に制御端末になるから ioctl は不要らしい
#ifdef TIOCSCTTY
    ioctl(slave_fd, TIOCSCTTY, 0);
#endif

    // 子プロセスの標準入力(0), 標準出力(1), 標準エラー出力(2) を Slave
    // に接続する
    dup2(slave_fd, STDIN_FILENO);
    dup2(slave_fd, STDOUT_FILENO);
    dup2(slave_fd, STDERR_FILENO);

    // 上の三つの dup2 で 0, 1, 2 番を新しく作った slave_fd で上書きしたから
    // 3番目は不要
    if (slave_fd > STDERR_FILENO) {
      ::close(slave_fd);
    }

    // シェルを実行
    char *args[] = {const_cast<char *>(shell), nullptr};
    execvp(args[0], args);

    // execvp が失敗した場合のみここに来る
    std::perror("execvp failed");
    ::_exit(1);
  } else {
    this->shell_pid = pid;
  }

  std::cout << "[INFO] Shell started (PID: " << pid << ")\n";
  if (pthread_create(&this->read_thread, nullptr, read_thread_entry, this) !=
      0) {
    std::perror("thread create failed");
    return;
  }
  this->is_running = true;
}

ImpPTY *ImpPTY::createPTY() {
  ImpPTY *pty = static_cast<ImpPTY *>(std::malloc(sizeof(ImpPTY)));
  if (pty == nullptr) {
    std::perror("malloc failed (PTY)");
    return nullptr;
  }

  // 例外を使わないので配置 new (-fno-exceptions)
  pty = new (pty) ImpPTY;
  pty->addRef();

  // すでに作成済みならそのまま返す
  // ----------------------------
  // pty master を作成
  // ----------------------------
  // open("/dev/ptmx", O_RDWR | O_NOCTTY) と同じ意味らしい
  pty->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  if (pty->master_fd < 0) {
    std::perror("posix_openpt failed");
    pty->release();
    return nullptr;
  }

  // PTY Slave 側のアクセス権限を設定し、アクセスを許可
  if (grantpt(pty->master_fd) < 0 || unlockpt(pty->master_fd) < 0) {
    std::perror("grantpt/unlockpt failed");
    pty->release();
    return nullptr;
  }

  // PTY Slave のデバイスファイルパス名を取得 (/dev/ttys00X のような文字列)
  char *tmp_slave_path = ptsname(pty->master_fd);
  if (!tmp_slave_path) {
    std::perror("ptsname failed");
    pty->release();
    return nullptr;
  }

  ::strncpy(pty->slave_path, tmp_slave_path, sizeof(pty->slave_path) - 1);
  pty->slave_path[sizeof(pty->slave_path) - 1] = '\0';

  // コピー後のパスが存在するかチェック
  // 0 で成功らしい
  if (access(pty->slave_path, F_OK) != 0) {
    std::perror("slave device file does not exist");
    pty->release();
    return nullptr;
  }

  // パイプを作成
  if (pipe(pty->read_thread_pipe) < 0) {
    std::perror("pipe failed");
    pty->release();
    return nullptr;
  }
  std::cout << "[INFO] PTY Master opened. Slave path: " << pty->slave_path
            << std::endl;
  return pty;
}
} // namespace

// ----------------------------
// Application の PTY 部分
// メンバ関数の実装
// ----------------------------
IPTY *IPTY::createPTY() {
  ImpPTY *pty = ImpPTY::createPTY();
  return pty;
}
