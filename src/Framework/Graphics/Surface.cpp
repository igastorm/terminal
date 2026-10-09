#include "Surface.hpp"

//  ========================================================
//
//  Surface
//
//  ========================================================

int Surface::release() {
  int ref_count = this->Object<ISurface>::release();
  if (ref_count == 0) {
    this->~Surface();
    return 0;
  }
  return ref_count;
}

Surface::Surface(IGraphicsDevice *device,
                             IObject *window_or_texture) {
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
