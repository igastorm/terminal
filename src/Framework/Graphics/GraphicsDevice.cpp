#include "GraphicsDevice.hpp"
#include <cstdlib>

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

int GraphicsDevice::addRef() { return ++this->ref_count; }

int GraphicsDevice::release() {
  if (--this->ref_count == 0) {
    this->~GraphicsDevice();
    free(this);
    return 0;
  }
  return this->ref_count;
}

GraphicsDevice::GraphicsDevice() { this->addRef(); }

GraphicsDevice::~GraphicsDevice() {}
