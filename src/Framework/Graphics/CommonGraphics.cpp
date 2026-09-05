#include "CommonGraphics.hpp"
#include <cstdlib>

//  ----------------------------
//  Render Pass
//  ----------------------------

//int CommonRenderPass::addRef() { return ++this->ref_count; }

//int CommonRenderPass::release() {
//  if (--this->ref_count == 0) {
//    this->~CommonRenderPass();
//    free(this);
//    return 0;
//  }
//  return this->ref_count;
//}

//  ----------------------------
//  Surface
//  ----------------------------

int CommonSurface::addRef() { return ++this->ref_count; }

int CommonSurface::release() {
  if (--this->ref_count == 0) {
    this->~CommonSurface();
    free(this);
    return 0;
  }
  return this->ref_count;
}

//  ----------------------------
//  Graphics Device
//  ----------------------------

int CommonGraphicsDevice::addRef() { return ++this->ref_count; }

int CommonGraphicsDevice::release() {
  if (--this->ref_count == 0) {
    this->~CommonGraphicsDevice();
    free(this);
    return 0;
  }
  return this->ref_count;
}
