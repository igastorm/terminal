#include "IApplication.hpp"
#include "PTY/IPTY.hpp"
#include <iostream>

class TerminalApp : public IAppHandler {
private:
  IWindow *window = nullptr;
  IPTY *pty = nullptr;

public:
  bool onInit(IApplication *appInstance) override {
    std::cout << "[INFO] onInit: Creating window and PTY...\n";
    this->window = appInstance->createWindow(800, 600, "Terminal");

    this->pty = IPTY::createPTY();
    if (this->pty != nullptr) {
      pty->startShell("/bin/zsh");
    }

    IGraphicsDevice* device = appInstance->createGraphicsDevice();
    ISurface* surface = device->createSurface(100, 100);
    surface->release();
    device->release();
    return true;
  }

  AppResult onEvent(IApplication *app, const Event &event) override {
    if (event.type == EventType::WindowCloseRequest) {
      return AppResult::Quit;
    }
    // キーボード入力のテスト
    if (event.type == EventType::TextInput) {
      if (this->pty != nullptr) {
        this->pty->writeInput(event.text.utf8, event.text.len);
      }
    }
    return AppResult::Continue;
  }

  void onQuit(IApplication *app) override {
    std::cout << "[INFO] onQuit: Cleaning up...\n";
    if (this->window) {
      this->window->release();
    }
    if (this->pty) {
      this->pty->release();
    }
  }
};

int appMain(int argc, char **argv, IApplication *appInstance) {
  TerminalApp app;
  appInstance->run(&app);
  return 0;
}
