#pragma once
#include "IGraphicsDevice.hpp"
#include "IRenderPass.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

class RenderPass : public IRenderPass {
protected:
  IGraphicsDevice *device = nullptr;

  RenderPass(IGraphicsDevice *);
  ~RenderPass();

public:
};
