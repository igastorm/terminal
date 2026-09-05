#include "IApplication.hpp"
#include "PTY/IPTY.hpp"
#include <iostream>

class TerminalApp : public IAppHandler {
private:
  IWindow *window = nullptr;
  IPTY *pty = nullptr;
  IGraphicsDevice *device = nullptr;
  ISurface *surface = nullptr;

  void createTerminalWindow(IApplication *appInstance) {
    if (this->window == nullptr) {
      this->window = appInstance->createWindow(800, 600, "Terminal");
    }

    if (this->pty == nullptr && this->window != nullptr) {
      this->pty = IPTY::createPTY(appInstance);
      if (this->pty != nullptr) {
        pty->startShell("/bin/zsh");
      }

      device = appInstance->createGraphicsDevice();
      surface = device->createSurface(100, 100);
      surface->bindToWindow(window);
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

    if (this->device != nullptr) {
      this->device->release();
      this->device = nullptr;
    }

    if (this->surface != nullptr) {
      this->surface->unbindWindow();
      this->surface->release();
      this->surface = nullptr;
    }
  }

  void processPTYInput(const Event &event) {
    if (this->pty != nullptr) {
      this->pty->writeInput(event.text.utf8, event.text.len);
    }
  }

public:
  bool onInit(IApplication *appInstance) override {
    std::cout << "[INFO] onInit: Creating window and PTY...\n";
    createTerminalWindow(appInstance);
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
      return AppResult::Continue;
    }

    if (event.type == EventType::UserEvent) {
      std::cout << "UserEvent\n";
     return AppResult::Continue; 
    }

    if (event.type == EventType::WindowExpose) {
      if (event.window == this->window) {
        device->render(surface, [](IRenderPass *, void *) -> void {}, nullptr);
      }
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
