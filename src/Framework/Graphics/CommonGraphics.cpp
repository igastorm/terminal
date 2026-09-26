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

ITexture* CommonFontAtlas::getTexture() {
  return this->texture;
}

int CommonFontAtlas::addRef() { return ++this->ref_count; }

int CommonFontAtlas::release() {
  if (--this->ref_count == 0) {
    this->~CommonFontAtlas();
    free(this);
    return 0;
  }
  return this->ref_count;
}

// コードポイントをハッシュ化
size_t CommonFontAtlas::hashCodepoint(std::uint32_t cp) const {
  // このようなハッシュ関数にするとなんか重複が少なくなるらしい
  // 原理はいまいちわからん
  return (cp * 2654435761u) & (HashEntry::HASH_SIZE - 1);
}

bool CommonFontAtlas::rewindCursor() {
  int cols_per_row = this->cols_per_row;
  if (cols_per_row == 0) {
    return false;
  }

  this->cursor_x = 0.0f;

  // 整数で切り上げを行う公式らしい
  int ascii_rows = (95 + cols_per_row - 1) / cols_per_row;
  this->cursor_y = ascii_rows * cell_height;
  return true;
}

// ハッシュテーブルに既に存在するか探すだけ
const HashEntry* CommonFontAtlas::findEntry(uint32_t code_point) const {
  const std::size_t start_idx = this->hashCodepoint(code_point);
  std::size_t idx = start_idx;

  while (this->glyph_hash_table[idx].codepoint != 0) {
    if (this->glyph_hash_table[idx].codepoint == code_point) {
      // 見つかった
      return &this->glyph_hash_table[idx];
    }
    idx = (idx + 1) & (HashEntry::HASH_SIZE - 1);
    if (idx == start_idx) {
      // 一周した（満タンかつ見つからなかった）
      break;
    }
  }
  
  // キャッシュに存在しない
  return nullptr;
}

CommonFontAtlas::CommonFontAtlas(IGraphicsDevice *device) {
  this->addRef();
  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
}

CommonFontAtlas::~CommonFontAtlas() {
  if (this->texture != nullptr) {
    this->texture->release();
    this->texture = nullptr;
  }
  if (this->device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
  if (this->on_demand_bitmap_data != nullptr) {
    std::free(this->on_demand_bitmap_data);
    this->on_demand_bitmap_data = nullptr;
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
