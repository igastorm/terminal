#include "RenderPass.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

RenderPass::RenderPass(IGraphicsDevice *device) : device(device) {
  if (this->device != nullptr) {
    this->device->addRef();
  }
}

RenderPass::~RenderPass() {
  if (device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
}
