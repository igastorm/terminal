#include "GraphicsDevice.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

int GraphicsDevice::release() {
  int ref_count = this->Object<IGraphicsDevice>::release();
  if (ref_count == 0) {
    this->~GraphicsDevice();
    return 0;
  }
  return ref_count;
}

GraphicsDevice::GraphicsDevice() { this->addRef(); }

GraphicsDevice::~GraphicsDevice() {}
