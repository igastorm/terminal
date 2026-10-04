#include "RenderPass.hpp"
#include <cstdlib>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

int RenderPass::addRef() { return ++this->ref_count; }

int RenderPass::release() {
  if (--this->ref_count == 0) {
    this->~RenderPass();
    free(this);
    return 0;
  }
  return this->ref_count;
}