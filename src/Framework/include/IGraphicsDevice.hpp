#pragma once
#include "IObject.hpp"
#include "IRenderPass.hpp"
#include "ISurface.hpp"

class IGraphicsDevice : public IObject {
public:
  virtual ~IGraphicsDevice() = default;

  virtual ISurface *createSurface(int, int) = 0;

  virtual bool render(ISurface *, RenderCallBack, void *,
                      const RenderPassDesc = IRenderPass::DEFAULT_DESC) = 0;
};
