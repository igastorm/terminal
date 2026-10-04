#include "Surface.hpp"
#include <cstdlib>

//  ========================================================
//
//  Surface
//
//  ========================================================

int Surface::addRef() { return ++this->ref_count; }

int Surface::release() {
  if (--this->ref_count == 0) {
    this->~Surface();
    free(this);
    return 0;
  }
  return this->ref_count;
}

Surface::Surface(IGraphicsDevice *device,
                             IObject *window_or_texture) {
  this->addRef();
  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
  if (window_or_texture != nullptr) {
    this->window_or_texture = window_or_texture;
    this->window_or_texture->addRef();
  }
}

Surface::~Surface() {
  if (device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
  if (window_or_texture != nullptr) {
    this->window_or_texture->release();
    this->window_or_texture = nullptr;
  }
}
