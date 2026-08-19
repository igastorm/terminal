#include "../Application/ImpApplication.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <new>
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

  // 元のターミナル設定を保存
  termios orig_termios;

  int master_fd = -1;
  char slave_path[256] = {0};

  void event_loop(void);
  // 親からの標準入力を行単位ではなく文字単位で受け取る設定
  void enable_raw_mode(void);
  void start_shell(const char *shell) override;
  void close(void);
  int release(void) override;
  int addRef(void) override;
  ImpPTY(void) = default;
  ~ImpPTY(void);

public:
  static ImpPTY *createPTY(void);
};

// ----------------------------
// メンバ関数の実装
// ----------------------------

int ImpPTY::addRef(void) { return ++this->ref_count; }

int ImpPTY::release(void) {
  if (--this->ref_count == 0) {
    this->~ImpPTY();
    free(this);
    return 0;
  }
  return this->ref_count;
}

ImpPTY::~ImpPTY(void) { this->close(); }

void ImpPTY::close(void) {
  if (master_fd >= 0) {
    ::close(master_fd);
    std::cout << std::endl << "Exit the terminal" << std::endl;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  } else {
    std::cout << std::endl
              << "The file descriptor does not exist." << std::endl;
  }
}

void ImpPTY::enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &orig_termios);

  termios raw = orig_termios;

  // raw モードになるように構造体を設定
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void ImpPTY::event_loop(void) {
  // ----------------------------
  // メインループ
  // ----------------------------
  // イベントの監視対象を2つ設定する
  pollfd fds[2];

  // どちらも POLLIN なのは外部からターミナルへの入力を監視するから
  // 監視対象 1: ユーザーのキーボード入力 (標準入力 STDIN_FILENO)
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  // 監視対象 2: シェルからの出力 (PTY Master)
  fds[1].fd = this->master_fd;
  fds[1].events = POLLIN;

  char buffer[1024];

  while (true) {

    // イベントを待機 (タイムアウトなし: -1)
    int ret = poll(fds, 2, -1);

    // 戻りはイベントが発生したファイル記述子の数
    if (ret < 0) {
      break;
    }

    // キーボードから入力があった場合、入力を読み込む
    if (fds[0].revents & POLLIN) {
      ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));

      // キー入力があるのに何も読み込まなかったら異常
      if (bytes_read <= 0) {
        break;
      }

      // 特殊な終了キーチェック (Ctrl + ']' ＝ ASCIIコード 29)
      // Ctrl + C はシェル側の終了コード
      // Ctrl + C
      // で端末が終了するとアプリを強制終了するとともに端末も終了することになる.
      // 緊急でターミナルを終了する.
      if (bytes_read == 1 && buffer[0] == 29) {
        break;
      }

      // PTY Master にそのまま送る (自分が管理する PTY)
      write(this->master_fd, buffer, bytes_read);
    }

    // シェル (PTY Master) から出力が届いた場合 -> 画面 (標準出力) に表示する
    if (fds[1].revents & POLLIN) {
      ssize_t bytes_read = read(this->master_fd, buffer, sizeof(buffer));
      if (bytes_read <= 0) {
        // シェルが終了した (exit など)
        break;
      }

      // 読み取った文字列をそのまま自分自身の標準出力へ流す
      write(STDOUT_FILENO, buffer, bytes_read);
    }
  }
}

void ImpPTY::start_shell(const char *shell) {
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

    // 親プロセスの Master fd は不要なので閉じる (参照カウンタを減らす)
    ::close(this->master_fd);

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
  }
  // --- 親プロセス（エミュレータ側）の処理 ---
  std::cout << "[INFO] Shell started (PID: " << pid
            << "). Entering main loop...\n";
  std::cout << "[INFO] Press Ctrl+] to exit.\n\n";

  // キー入力を1バイトずつ即時取得するため端末を Raw モードに切り替え
  enable_raw_mode();
  this->event_loop();
}

ImpPTY *ImpPTY::createPTY(void) {
  ImpPTY *pty = static_cast<ImpPTY *>(std::malloc(sizeof(ImpPTY)));
  if (pty == nullptr) {
    std::perror("malloc failed (PTY)");
    return nullptr;
  }

  // 例外を使わないので配置 new (-fno-exceptions)
  pty = new (pty) ImpPTY;
  pty->addRef();
  tcgetattr(STDIN_FILENO, &pty->orig_termios);

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
  std::cout << "[INFO] PTY Master opened. Slave path: " << pty->slave_path
            << std::endl;
  return pty;
}
} // namespace

// ----------------------------
// Application の PTY 部分
// メンバ関数の実装
// ----------------------------
IPTY *ImpApplication::createPTY(void) {
  ImpPTY *pty = ImpPTY::createPTY();
  return pty;
}
