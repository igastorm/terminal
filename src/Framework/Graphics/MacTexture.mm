#include "../Application/ApplicationTemplate.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "GraphicsTemplate.hpp"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "MacGraphics.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Texture
//
//  ========================================================

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

  texture = new (texture) MacTexture;
  texture->addRef();

  texture->device = device;
  device->addRef();

  texture->width = width;
  texture->height = height;

  texture->format = desc.format;

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

    texture->data.texture =
        [static_cast<MacGraphicsDevice *>(device)->getPlatformData().device
            newTextureWithDescriptor:texture_desc];
    [texture_desc release];
    if (texture->data.texture == nil) {
      texture->release();
      return nullptr;
    }
  }

  return texture;
}

template <> Texture::~TextureTemplate<TextureData>() {
  @autoreleasepool {
    if (this->data.texture != nil) {
      [this->data.texture release];
      this->data.texture = nil;
    }
    if (this->device != nullptr) {
      this->device->release();
      this->device = nullptr;
    }
  }
}

template <> TextureData Texture::getPlatformData() const { return this->data; }

template <>
bool Texture::upload(const void *pixels, size_t bytes, size_t bytes_per_row) {
  if (this->data.texture == nil || pixels == nullptr) {
    return false;
  }

  // 与えられたデータが要件を満たしていなければ弾く
  size_t bpp =
      (this->format == TextureFormat::Mono) ? 1 : sizeof(std::uint32_t);
  if (bytes_per_row / bpp != static_cast<size_t>(this->width)) {
    return false;
  }
  if (bytes / bytes_per_row != static_cast<size_t>(this->height)) {
    return false;
  }

  @autoreleasepool {
    MTLRegion region =
        MTLRegionMake2D(0, 0, this->width, this->height);
    [this->data.texture replaceRegion:region
                          mipmapLevel:0
                            withBytes:pixels
                          bytesPerRow:bytes_per_row];
    return true;
  }
}
