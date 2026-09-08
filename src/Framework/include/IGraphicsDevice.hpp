#pragma once
#include "IObject.hpp"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"

class IGraphicsDevice : public IObject {
public:
  virtual ~IGraphicsDevice() = default;

  virtual ISurface *createSurface() = 0;

  virtual ITexture* createTexture(int, int, TextureDrawable = TextureDrawable::Disable) = 0;

  virtual bool render(ISurface *, RenderCallBack, void *,
                      const RenderPassDesc = IRenderPass::DEFAULT_DESC) = 0;
};
