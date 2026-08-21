#include "IApplication.hpp"
#include <iostream>

class TerminalApp : public IAppHandler {
private:
  IWindow *window = nullptr;

public:
  bool onInit(IApplication *appInstance) override {
    std::cout << "[INFO] onInit: Creating window and PTY...\n";
    this->window = appInstance->createWindow(800, 600, "Terminal");
    return true;
  }
  AppResult onEvent(IApplication *app, const Event &event) override {
    if (event.type == EventType::WindowCloseRequest) {
      event.window->release();
      return AppResult::Quit;
    }
    return AppResult::Continue;
  }
  void onQuit(IApplication *app) override {
    std::cout << "[INFO] onQuit: Cleaning up...\n";
    if (this->window)
      this->window->release();
  }
};

int appMain(int argc, char **argv, IApplication *appInstance) {
  // IWindow *window = appInstance->createWindow(200, 200, "test");
  // while (true) {
  //   appInstance->processEvent();
  //   if (window->shouldClose()) {
  //     window->release();
  //     break;
  //   }
  // }
  //  将来タブ機能をつけるために複数の PTY を作れるようにする
  //  つまり GUI 化した時に接続する PTY を指定する仕組みが必要
  //  IPTY *pty0 = appInstance->createPTY();
  //  if (pty0) {
  //   char shell[] = "/bin/zsh";
  //   pty0->start_shell(shell);
  //   pty0->release();
  // }
  TerminalApp app;
  appInstance->run(&app);
  return 0;
}
