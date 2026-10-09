#include "Texture.hpp"
//  ========================================================
//
//  Texture
//
//  ========================================================

int Texture::release() {
  int ref_count = this->Object<ITexture>::release();
  if (ref_count == 0) {
    this->~Texture();
    return 0;
  }
  return ref_count;
}

TextureFormat Texture::getFormat() { return this->format; }

int Texture::getWidth() { return this->width; }

int Texture::getHeight() { return this->height; }

Texture::Texture(IGraphicsDevice *device, int w, int h,
                             TextureFormat format) {
  this->width = w;
  this->height = h;
  this->format = format;

  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
}

Texture::~Texture() {
  this->width = 0;
  this->height = 0;
  this->format = TextureFormat::Color;

  if (this->device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
}