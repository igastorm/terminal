#include "PTY.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <new> // IWYU pragma: keep
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>

//  ========================================================
//
//  メンバ関数の実装
//
//  ========================================================

int PTY::addRef() { return ++this->ref_count; }

int PTY::release() {
  if (--this->ref_count == 0) {
    this->~PTY();
    free(this);
    return 0;
  }
  return this->ref_count;
}

void PTY::normalExitMsgHelper(int status) {
  // WEXITSTATUS: 子プロセスの終了コードを得る
  int exit_code = WEXITSTATUS(status);
  std::cout << "\n[INFO] Shell exited normally with code: " << exit_code
            << std::endl;
}

PTY::~PTY() { this->destrustor_helper(); }

void PTY::destrustor_helper() {
  if (this->read_thread_pipe[1] >= 0) {
    const char c = 'q';
    // シェルの終了通知 (poll によるスリープから叩き起こす)
    // すでにシェルが終了していれば読み取りスレッドはループを抜けて終了しているはずなので空振りするだけ
    write(this->read_thread_pipe[1], &c, sizeof(c));

    // 読み込みスレッドが生成される前にエラー等で終了するときに join
    // するとまずいのでフラグから判定
    // このフラグは startShell で読み取りスレッドを起動した時に ture
    // にセットしている
    if (this->is_running) {
      this->is_running = false;
      pthread_join(this->read_thread,
                   nullptr); // 読み取りスレッドの終了を待つ
      std::cout << "\n[INFO] PTY Closed.\n";
    }

    // 終了通知用パイプの入口と出口を閉じる
    if (this->read_thread_pipe[0] >= 0) {
      close(this->read_thread_pipe[0]);
      this->read_thread_pipe[0] = -1;
    }
    if (this->read_thread_pipe[1] >= 0) {
      close(this->read_thread_pipe[1]);
      this->read_thread_pipe[1] = -1;
    }
  }

  // シェルの後始末
  // シェルが通常終了した時は shell_pid はすでに 0 にしてある
  // kill で終了要求した時は shell_pid はまだ 0 ではない (優しい kill)
  // →確実に即時終了するとは限らないので
  if (this->shell_pid > 0) {
    int status = 0;

    // 一旦, ブロッキングせずに優しい kill で終了したか見る
    if (waitpid(this->shell_pid, &status, WNOHANG) == 0) {
      // 終了していなかったら強い kill で即座に止める
      // 普通ここに来る時は release した時だから readLoop
      // 側で終了してここに来るまでの間に終了してるはず
      // だからあえて実装位置を離してる
      // それでも終了していなかった時にここで強制終了する
      kill(this->shell_pid, SIGKILL);

      // 即座に kill するからブロッキングしもいい
      waitpid(this->shell_pid, &status, 0);
    }

    // 完全に子プロセスは完全に停止しているはずだからここで
    // 終了方法に応じてログを出す
    if (WIFEXITED(status)) {
      // 優しい kill で済んだ場合
      normalExitMsgHelper(status);
    } else if (WIFSIGNALED(status)) {
      // 優しい kill が間に合わなくて強制 kill したとき
      int sig = WTERMSIG(status);
      std::cout << "\n[INFO] Shell killed by signal: " << strsignal(sig)
                << std::endl;
    }
    this->shell_pid = 0;
  }
  if (master_fd >= 0) {
    close(master_fd);
    this->master_fd = -1;
    std::cout << std::endl << "Exit the terminal" << std::endl;
  } else {
    std::cout << std::endl
              << "The file descriptor does not exist." << std::endl;
  }
}

// キー入力を PTY に流す
void PTY::writeInput(const void *data, size_t len) {
  if (this->is_running) {
    if (this->master_fd >= 0 && data != nullptr && len > 0) {
      write(this->master_fd, data, len);
    }
  }
}

// readLoop を起動する
// pthread_create が要求するシグネチャに合わせるために経由する
void *PTY::readThreadEntry(void *args) {
  PTY *pty = reinterpret_cast<PTY *>(args);
  pty->readLoop();
  return nullptr;
}

void PTY::readLoop() {
  // シェルの起動が失敗したら何かしらのメッセージかダイアログを出すべき
  // read
  // では読み取った分だけシークするので溢れたら自動的に複数に分割して読み込めるから
  // 1024 あればいいと思われる
  // ただしエスケープシーケンスの途中で切れる可能性も考慮する必要がある
  char buffer[1024];
  pollfd pfds[2] = {};
  pfds[0].fd = this->read_thread_pipe[0];
  pfds[0].events = POLLIN;
  pfds[1].fd = this->master_fd;
  pfds[1].events = POLLIN; // masterfd への入力 (つまりシェルの出力) を監視

  while (true) {
    int ret = poll(pfds, 2, -1);

    // 終了要求で叩き起こされた
    if (ret < 0 || (pfds[0].revents & POLLIN)) {
      if (this->shell_pid > 0) {
        kill(this->shell_pid, SIGHUP);
      }
      break;
    }

    // シェルから入力 (シェルの出力) が来た
    if (pfds[1].revents & POLLIN) {
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
        // 子プロセスの終了を待つ
        waitpid(this->shell_pid, &status, 0);

        // WIFEXITED: 子プロセスが正常終了した時に真となる
        if (WIFEXITED(status)) {
          normalExitMsgHelper(status);
        }
        this->shell_pid = 0;
        break;
      }

      // main 側へ通知
      this->appInstance->postEvent();

      // ここで ANSI パーサを呼んで TextGrid を生成するかも
      write(STDOUT_FILENO, buffer, bytes_read);
    }
  }
}

void PTY::startShell(const char *shell) {
  // シェルを起動 (fork する)
  // fork すると同じ内容のサブプロセスが作られる (実行位置は fork() の直後)
  // つまりメモリ空間がクローンされる
  // また, ファイル記述子の実態はカーネルが管理しており,
  // クローン時にカーネル内部では参照カウントのようなものがインクリメントされるので
  // 親から引き継がれた余分なファイル記述子は閉じる必要がある
  // 尚, ファイル記述子の番号自体はプロセス固有であり,
  // 実体と結びつけるテーブルはカーネル側に存在する
  // fork
  // はプロセス空間をクローンするということはそのテーブルについても浅いコピーを
  // することになるので結果的に親と同じテーブルを参照することになる
  // 子プロセスの実行位置は fork の直後
  pid_t pid = fork();
  if (pid < 0) {
    std::perror("fork failed");
    return;
  }

  // 子プロセスの場合は pid == 0 が返ってくる
  // つまり子プロセスのみで行いたい処理はここに書けば良い
  if (pid == 0) {
    // 子プロセス側の処理 (zsh になる予定のプロセス)

    // デバッグ用環境変数を除去
    unsetenv("MallocStackLogging");
    unsetenv("MallocStackLoggingNoCompact");
    unsetenv("ASAN_OPTIONS");
    unsetenv("MTL_DEBUG_LAYER");

    // 親プロセスの Master fd は不要なので閉じる (参照カウンタを減らす)
    close(this->master_fd);

    // 同様に親プロセスのパイプは不要なので閉じる (参照カウンタを減らす)
    close(this->read_thread_pipe[0]);
    close(this->read_thread_pipe[1]);

    // 新しいセッションを作成し, プロセスグループのリーダーになる
    // 標準ターミナルとの縁を切って無理やり自作 pty
    // に配管を繋ぎかえるイメージらしい
    // プロセスグループのリーダしか制御端末を取得できないルールなので新しいセッションを作ってボスになる必要がある
    // これがなくても動いているように見えるが実際には ioctl
    // がエラーを返していることから制御端末が別の端末なのに標準入出力がこの端末というカオス状態になる
    setsid();

    // slave 側のファイル記述子を開く
    // O_NOCTTY をつけてないので slave がこのプロセスの親端末になる
    int slave_fd = open(this->slave_path, O_RDWR);
    if (slave_fd < 0) {
      std::perror("open slave failed");
      _exit(1);
    }

    // この Slave をプロセスの制御端末 (Controlling Terminal) に設定する
    // BSD 系 Unix では open で O_NOCTTY
    // を付けなくても自動的に制御端末にならないから必要らしい
    // 逆に SystemV 系だと自動的に制御端末になるから ioctl は不要らしい
#ifdef TIOCSCTTY
    int result = ioctl(slave_fd, TIOCSCTTY, 0);
    if (result < 0) {
      std::perror("ioctl failed");
      _exit(1);
    }
#endif

    // 子プロセスの標準入力(0), 標準出力(1), 標準エラー出力(2) を Slave
    // に接続する
    // dup2: 第二引数のファイル記述子の番号で第一引数のファイル記述子を複製する
    // 元のファイル記述子は close される
    // 3 種類のファイル記述子全て slave
    // というデバイスに繋いでいるが以下のようなエイリアスみたいなもん
    // 標準入力用の参照
    // 標準出力用の参照
    // 標準エラー出力用の参照
    dup2(slave_fd, STDIN_FILENO);
    dup2(slave_fd, STDOUT_FILENO);
    dup2(slave_fd, STDERR_FILENO);

    // 上の三つの dup2 で 0, 1, 2 番を新しく作った slave_fd で上書きしたから
    // 3番目は不要
    // 通常は 3 だがデバッガを使うと 4 とかになったり, たまたま slave_fd が 0
    // だった時などにまずいのでこの if 文が必要
    if (slave_fd > STDERR_FILENO) {
      close(slave_fd);
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
  if (pthread_create(&this->read_thread, nullptr, readThreadEntry, this) != 0) {
    std::perror("thread create failed");
    return;
  }
  this->is_running = true;
}

PTY *PTY::createPTY(IApplication *appInstance) {
  PTY *pty = static_cast<PTY *>(std::malloc(sizeof(PTY)));
  if (pty == nullptr) {
    std::perror("malloc failed (PTY)");
    return nullptr;
  }

  pty = new (pty) PTY;

  // pty master を作成
  // open("/dev/ptmx", O_RDWR | O_NOCTTY) と同じ意味らしい
  // ただし, /dev/ptmx は特殊デバイスで master と slave の PTY ペアが作られる
  // O_RDWR: 読み書き用に開く
  // O_NOCTTY: 開いた端末をこのプログラムの制御端末にしない
  // 今作りたいのは端末Aなのにその端末Aを制御する端末Bを新しく作ってしまうと訳のわからないことになる
  // 例えば Ctrl+C
  // を押した時に端末Bが終了するがそれと同時に端末Aも停止するので全体がクラッシュする
  // masterfd は端末で, slavefd はシェルの親端末を指し, パイプで繋がっている
  // (つまりセット)
  // master_fd は通常, 3 になるはずだがデバッガ経由で実行した時は 4 とかになる
  // 通常は次のようになっており, 小さい番号が優先して割り当てられる
  // 0: STDIN_FILENO:   標準入力
  // 1: STDOUT_FILENO:  標準出力
  // 2: STDERR_FILENO:  標準エラー出力
  // 3: master_fd:      擬似端末
  pty->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  if (pty->master_fd < 0) {
    std::perror("posix_openpt failed");
    pty->release();
    return nullptr;
  }

  // PTY Slave 側のアクセス権限を設定し、アクセスを許可
  // grantpt:
  // master に対応する slave の所有者 ID をこのプロセスの UID に設定
  // (通常はユーザと一致)
  // unlockpt:
  // mster に対応する slave のロックを解除してアクセス可能にする
  // 所有者を設定しないと危険だから初めはロックされてるらしい
  if (grantpt(pty->master_fd) < 0 || unlockpt(pty->master_fd) < 0) {
    std::perror("grantpt/unlockpt failed");
    pty->release();
    return nullptr;
  }

  // slave のデバイスファイルパス名を取得 (/dev/ttys00X のような文字列)
  char *tmp_slave_path = ptsname(pty->master_fd);
  if (!tmp_slave_path) {
    std::perror("ptsname failed");
    pty->release();
    return nullptr;
  }

  strncpy(pty->slave_path, tmp_slave_path, sizeof(pty->slave_path) - 1);
  pty->slave_path[sizeof(pty->slave_path) - 1] = '\0';

  // コピー後のパスが存在するかチェック
  // 0 で成功らしい
  if (access(pty->slave_path, F_OK) != 0) {
    std::perror("slave device file does not exist");
    pty->release();
    return nullptr;
  }

  // パイプを作成
  // メインスレッドと読み取りスレッドを繋ぐパイプ
  // 読み取りスレッドがスリープ中にこのパイプ経由で何かデータを書き込めばスレッドを叩き起こして終了処理をさせられる
  // ビジーウェイトの回避
  if (pipe(pty->read_thread_pipe) < 0) {
    std::perror("pipe failed");
    pty->release();
    return nullptr;
  }

  pty->appInstance = appInstance;

  std::cout << "[INFO] PTY Master opened. Slave path: " << pty->slave_path
            << std::endl;
  return pty;
}

//  ========================================================
//
//  ファクトリ
//
//  ========================================================
IPTY *IPTY::createPTY(IApplication *appInstance) {
  PTY *pty = PTY::createPTY(appInstance);
  return pty;
}
