#include "GraphicsDevice.hpp"
#include "../Application/Application.hpp"
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

GraphicsDevice::GraphicsDevice(IApplication *appInstance) {
  this->addRef();
  if (appInstance != nullptr) {
    this->appInstance = appInstance;
    static_cast<Application *>(this->appInstance)->addRef();
  }
}

GraphicsDevice::~GraphicsDevice() {
  if (this->appInstance != nullptr) {
    static_cast<Application *>(this->appInstance)->release();
  }
}
