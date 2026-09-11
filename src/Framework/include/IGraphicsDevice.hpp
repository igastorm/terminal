#pragma once
#include "IObject.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"
#include "IWindow.hpp"

class IGraphicsDevice : public IObject {
public:
  virtual ~IGraphicsDevice() = default;

  [[nodiscard]] virtual ISurface *createSurfaceFromWindow(IWindow *) = 0;
  [[nodiscard]] virtual ISurface* createSurfaceFromTexture(ITexture*) = 0;

  virtual ITexture *
  createTexture(int, int, TextureDrawable = TextureDrawable::Disable) = 0;
};
