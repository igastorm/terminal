#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
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

MacTexture *MacTexture::createMacTexture(MacGraphicsDevice *device, int width,
                                         int height,
                                         TextureDrawable drawable_flag) {
  MacTexture *texture =
      static_cast<MacTexture *>(std::malloc(sizeof(MacTexture)));
  if (texture == nullptr) {
    std::perror("malloc failed (createTexture)");
    return nullptr;
  }

  texture = new (texture) MacTexture;
  texture->addRef();

  texture->data.device = device;
  device->addRef();

  texture->data.width = width;
  texture->data.height = height;

  @autoreleasepool {
    MTLTextureDescriptor *texture_desc = [[MTLTextureDescriptor alloc] init];
    if (texture_desc == nil) {
      texture->release();
      return nullptr;
    }

    texture_desc.pixelFormat = MTLPixelFormatBGRA8Unorm;
    texture_desc.width = width;
    texture_desc.height = height;
    // 縮小した画像をあらかじめ生成する設定らしい
    // 使わないので 1
    texture_desc.mipmapLevelCount = 1;

    // texture_desc.storageMode

    if (drawable_flag == TextureDrawable::Enable) {
      texture_desc.usage =
          MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    } else {
      texture_desc.usage = MTLTextureUsageShaderRead;
    }

    texture->data.texture = [device->getPlatformData().device
        newTextureWithDescriptor:texture_desc];
    [texture_desc release];
    if (texture->data.texture == nil) {
      texture->release();
      return nullptr;
    }
  }

  return texture;
}

template <> ImpTexture::~ImpTextureTemplate<ImpTextureData>() {
  @autoreleasepool {
    if (this->data.texture != nil) {
      [this->data.texture release];
      this->data.texture = nil;
    }
    if (this->data.device != nullptr) {
      this->data.device->release();
      this->data.device = nullptr;
    }
  }
}

template <> ImpTextureData ImpTexture::getPlatformData() const {
  return this->data;
}
