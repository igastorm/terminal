#include "IApplication.hpp"
#include "PTY/IPTY.hpp"
#include <iostream>

class TerminalApp : public IAppHandler {
private:
  IWindow *window = nullptr;
  IPTY *pty = nullptr;
  IGraphicsDevice *device = nullptr;
  ISurface *surface = nullptr;
  ITexture *texture = nullptr;
  ISurface *texture_surface = nullptr;

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
      surface = device->createSurfaceFromWindow(this->window);
      texture = device->createTexture(
          800, 600, {TextureDrawable::Enable, TextureFormat::Color});
      uint32_t green_pixels[800 * 600];
      for (int i = 0; i < 800 * 600; ++i) {
        green_pixels[i] = 0xFF00FF00; // 不透明な黒
      }
      texture->upload(green_pixels, sizeof(green_pixels),
                      800 * sizeof(uint32_t));
      texture_surface = device->createSurfaceFromTexture(texture);
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
      this->surface->release();
      this->surface = nullptr;
    }

    if (this->texture != nullptr) {
      this->texture->release();
      this->texture = nullptr;
    }

    if (this->texture_surface != nullptr) {
      this->texture_surface->release();
      this->texture_surface = nullptr;
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
    // ITexture* tex = device->createTexture(100, 100);
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
        texture_surface->render(
            [](IRenderPass *pass, void *arg) -> void {
              float x = 50.0f;
              float y = 50.0f;
              float w = 300.0f;
              float h = 200.0f;

              Vertex quad1[6] = {
                  // 三角形 1
                  {{x, y}, 0xFFFF0000},     // 左下 (赤)
                  {{x + w, y}, 0xFF00FF00}, // 右下 (緑)
                  {{x, y + h}, 0xFF0000FF}, // 左上 (青)

                  // 三角形 2
                  {{x, y + h}, 0xFF0000FF},     // 左上 (青)
                  {{x + w, y}, 0xFF00FF00},     // 右下 (緑)
                  {{x + w, y + h}, 0xFFFFFF00}, // 右上 (黄)
              };

              x = 200.0f;
              y = 100.0f;
              w = 300.0f;
              h = 200.0f;

              Vertex quad2[6] = {
                  // 三角形 1
                  {{x, y}, 0x80FF0000},     // 左下 (赤)
                  {{x + w, y}, 0x8000FF00}, // 右下 (緑)
                  {{x, y + h}, 0x800000FF}, // 左上 (青)

                  // 三角形 2
                  {{x, y + h}, 0x800000FF},     // 左上 (青)
                  {{x + w, y}, 0x8000FF00},     // 右下 (緑)
                  {{x + w, y + h}, 0x80FFFF00}, // 右上 (黄)
              };
              pass->drawVertices(quad1, 6);
              pass->drawVertices(quad2, 6);
            },
            nullptr, {false, 0xFF1F1F1F, FrameDropping::Disable});
        surface->render(
            [](IRenderPass *pass, void *arg) -> void {
              ITexture *tex = static_cast<ITexture *>(arg);
              // 二分の一に縮小表示されることになる
              float x = 50.0f;
              float y = 50.0f;
              float w = 400.0f;
              float h = 300.0f;

              VertexTex quad[6] = {
                  {{x, y}, {0.0f, 0.0f}, 0xFFFFFFFF},     // 左上
                  {{x + w, y}, {1.0f, 0.0f}, 0xFFFFFFFF}, // 右上
                  {{x, y + h}, {0.0f, 1.0f}, 0xFFFFFFFF}, // 左下

                  {{x, y + h}, {0.0f, 1.0f}, 0xFFFFFFFF},     // 左下
                  {{x + w, y}, {1.0f, 0.0f}, 0xFFFFFFFF},     // 右上
                  {{x + w, y + h}, {1.0f, 1.0f}, 0xFFFFFFFF}, // 右下
              };
              pass->drawVerticesTex(tex, quad, 6);
            },
            this->texture);
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
