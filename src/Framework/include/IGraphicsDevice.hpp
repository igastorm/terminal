#pragma once
#include "IFontAtlas.hpp"
#include "IObject.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"
#include "IWindow.hpp"

class IGraphicsDevice : public IObject {
public:
  virtual ~IGraphicsDevice() = default;

  [[nodiscard]] virtual ISurface *createSurfaceFromWindow(IWindow *) = 0;
  [[nodiscard]] virtual ISurface *createSurfaceFromTexture(ITexture *) = 0;

  // ひとまず, 白黒かつレンダーターゲットはエラーにしておく
  [[nodiscard]] virtual ITexture *
  createTexture(int, int, const TextureDesc = ITexture::DEFAULT_DESC) = 0;

  [[nodiscard]] virtual ITexture *createFontTexture(const char *, int) = 0;
  [[nodiscard]] virtual IFontAtlas *createFontAtlas(const char *, float) = 0;
};
