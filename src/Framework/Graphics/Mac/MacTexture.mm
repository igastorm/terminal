#include "MacTexture.h"
#include "MacGraphicsDevice.h"
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Texture
//
//  ========================================================

MacTexture::MacTexture(IGraphicsDevice *device, int w, int h,
                       TextureFormat format)
    : Texture(device, w, h, format) {}

MacTexture *MacTexture::createMacTexture(IGraphicsDevice *device, int width,
                                         int height, const TextureDesc desc) {
  // 対応可能 (pipeline_state の用意がめんどくさすぎる) だが現時点では,
  // エラーにしておく
  if (desc.drawable_flag == TextureDrawable::Enable &&
      desc.format == TextureFormat::Mono) {
    return nullptr;
  }

  if (device == nullptr) {
    return nullptr;
  }

  MacTexture *texture =
      static_cast<MacTexture *>(std::malloc(sizeof(MacTexture)));
  if (texture == nullptr) {
    std::perror("malloc failed (createTexture)");
    return nullptr;
  }

  texture = new (texture) MacTexture(device, width, height, desc.format);
  // コンストラクタに任せるようにした
  // texture->addRef();

  // texture->device = device;
  // device->addRef();

  // texture->width = width;
  // texture->height = height;

  // texture->format = desc.format;

  @autoreleasepool {
    MTLTextureDescriptor *texture_desc = [[MTLTextureDescriptor alloc] init];
    if (texture_desc == nil) {
      texture->release();
      return nullptr;
    }

    texture_desc.pixelFormat = desc.format == TextureFormat::Color
                                   ? MTLPixelFormatBGRA8Unorm
                                   : MTLPixelFormatR8Unorm;
    texture_desc.width = width;
    texture_desc.height = height;
    // 縮小した画像をあらかじめ生成する設定らしい
    // 使わないので 1
    texture_desc.mipmapLevelCount = 1;

    // texture_desc.storageMode

    if (desc.drawable_flag == TextureDrawable::Enable) {
      texture_desc.usage =
          MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    } else {
      texture_desc.usage = MTLTextureUsageShaderRead;
    }

    texture->data.mtl_texture =
        [static_cast<MacGraphicsDevice *>(device)->getPlatformData().mtl_device
            newTextureWithDescriptor:texture_desc];
    [texture_desc release];
    if (texture->data.mtl_texture == nil) {
      texture->release();
      return nullptr;
    }
  }

  return texture;
}

MacTexture::~MacTexture() {
  @autoreleasepool {
    if (this->data.mtl_texture != nil) {
      [this->data.mtl_texture release];
      this->data.mtl_texture = nil;
    }
    // デストラクタに任せるようにした
    // if (this->device != nullptr) {
    //   this->device->release();
    //   this->device = nullptr;
    // }
  }
}

template <>
bool Texture::upload(const void *pixels, size_t bytes, size_t bytes_per_row,
                     TextureDataRegion rect) {
  // 与えられたデータが要件を満たしていなければ弾く
  if (this->data.mtl_texture == nil || pixels == nullptr) {
    return false;
  }

  if (rect.x < 0 || rect.y < 0 || rect.width < 0 || rect.height < 0) {
    return false;
  }

  if (rect.width + rect.x <= 0 || this->width < rect.width + rect.x) {
    return false;
  }

  if (rect.height + rect.y <= 0 || this->height < rect.height + rect.y) {
    return false;
  }

  size_t bpp = (this->format == TextureFormat::Mono) ? sizeof(std::uint8_t)
                                                     : sizeof(std::uint32_t);
  if (bytes_per_row / bpp != static_cast<size_t>(rect.width)) {
    return false;
  }
  if (bytes / bytes_per_row != static_cast<size_t>(rect.height)) {
    return false;
  }

  @autoreleasepool {
    MTLRegion region = MTLRegionMake2D(rect.x, rect.y, rect.width, rect.height);
    [this->data.mtl_texture replaceRegion:region
                              mipmapLevel:0
                                withBytes:pixels
                              bytesPerRow:bytes_per_row];
    return true;
  }
}
