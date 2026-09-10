#pragma once
#include "IObject.hpp"
#include "IRenderPass.hpp"

class ISurface : public IObject {
public:
  virtual ~ISurface() = default;
  virtual bool render(RenderCallBack, void *,
                      const RenderPassDesc = IRenderPass::DEFAULT_DESC) = 0;
};
