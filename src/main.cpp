#include "IApplication.hpp"
#include "PTY/IPTY.hpp"
#include <iostream>

class TerminalApp : public IAppHandler {
private:
  IWindow *window = nullptr;
  IPTY *pty = nullptr;

  void createTerminalWindow(IApplication *appInstance) {
    if (this->window == nullptr) {
      this->window = appInstance->createWindow(800, 600, "Terminal");
    }

    if (this->pty == nullptr) {
      this->pty = IPTY::createPTY();
      if (this->pty != nullptr) {
        pty->startShell("/bin/zsh");
      }
    }
  }

  void destroyTerminalWindow() {
    if (this->window != nullptr) {
      this->window->release();
      this->window = nullptr;
    }

    if (this->pty != nullptr) {
      this->pty->release();
      this->pty = nullptr;
    }
  }

  void processPTYInput(const Event& event) {
    if (this->pty != nullptr) {
      this->pty->writeInput(event.text.utf8, event.text.len);
    }
  }

public:
  bool onInit(IApplication *appInstance) override {
    std::cout << "[INFO] onInit: Creating window and PTY...\n";
    createTerminalWindow(appInstance);

    // IGraphicsDevice* device = appInstance->createGraphicsDevice();
    // ISurface* surface = device->createSurface(100, 100);
    // surface->release();
    // device->release();
    return true;
  }

  AppResult onEvent(IApplication *appInstance, const Event &event) override {
    if (event.type == EventType::WindowCloseRequest) {
      destroyTerminalWindow();
      return AppResult::Continue;
    }

    if (event.type == EventType::AppReopen) {
      createTerminalWindow(appInstance);
      return AppResult::Continue;
    }

    // キーボード入力のテスト
    if (event.type == EventType::TextInput) {
      processPTYInput(event);
    }
    return AppResult::Continue;
  }

  void onQuit(IApplication *app) override {
    std::cout << "[INFO] onQuit: Cleaning up...\n";
    destroyTerminalWindow();
  }
};

int appMain(int argc, char **argv, IApplication *appInstance) {
  TerminalApp app;
  appInstance->run(&app);
  return 0;
}
