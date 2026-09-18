#include "CommonGraphics.hpp"
#include "../Application/CommonApplication.hpp"
#include <cstdlib>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

int CommonRenderPass::addRef() { return ++this->ref_count; }

int CommonRenderPass::release() {
  if (--this->ref_count == 0) {
    this->~CommonRenderPass();
    free(this);
    return 0;
  }
  return this->ref_count;
}

//  ========================================================
//
//  Font Atlas
//
//  ========================================================

int CommonFontAtlas::addRef() { return ++this->ref_count; }

int CommonFontAtlas::release() {
  if (--this->ref_count == 0) {
    this->~CommonFontAtlas();
    free(this);
    return 0;
  }
  return this->ref_count;
}

CommonFontAtlas::CommonFontAtlas(IGraphicsDevice *device) {
  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
}

CommonFontAtlas::~CommonFontAtlas() {
  if (this->device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
}

//  ========================================================
//
//  Texture
//
//  ========================================================

int CommonTexture::addRef() { return ++this->ref_count; }

int CommonTexture::release() {
  if (--this->ref_count == 0) {
    this->~CommonTexture();
    free(this);
    return 0;
  }
  return this->ref_count;
}

TextureFormat CommonTexture::getFormat() { return this->format; }

int CommonTexture::getWidth() { return this->width; }

int CommonTexture::getHeight() { return this->height; }

CommonTexture::CommonTexture(IGraphicsDevice *device, int w, int h,
                             TextureFormat format) {
  this->width = w;
  this->height = h;
  this->format = format;

  this->addRef();
  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
}

CommonTexture::~CommonTexture() {
  this->width = 0;
  this->height = 0;
  this->format = TextureFormat::Color;

  if (this->device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
}

//  ========================================================
//
//  Surface
//
//  ========================================================

int CommonSurface::addRef() { return ++this->ref_count; }

int CommonSurface::release() {
  if (--this->ref_count == 0) {
    this->~CommonSurface();
    free(this);
    return 0;
  }
  return this->ref_count;
}

CommonSurface::CommonSurface(IGraphicsDevice *device,
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

CommonSurface::~CommonSurface() {
  if (device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
  if (window_or_texture != nullptr) {
    this->window_or_texture->release();
    this->window_or_texture = nullptr;
  }
}

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

int CommonGraphicsDevice::addRef() { return ++this->ref_count; }

int CommonGraphicsDevice::release() {
  if (--this->ref_count == 0) {
    this->~CommonGraphicsDevice();
    free(this);
    return 0;
  }
  return this->ref_count;
}

CommonGraphicsDevice::CommonGraphicsDevice(IApplication *appInstance) {
  this->addRef();
  if (appInstance != nullptr) {
    this->appInstance = appInstance;
    static_cast<CommonApplication *>(this->appInstance)->addRef();
  }
}

CommonGraphicsDevice::~CommonGraphicsDevice() {
  if (this->appInstance != nullptr) {
    static_cast<CommonApplication *>(this->appInstance)->release();
  }
}
