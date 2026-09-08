#include "MacGraphics.h"
#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
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

MacTexture *
MacTexture::createMacTexture(ImpGraphicsDevice *device, int width,
                                   int height, TextureDrawable drawable_flag) {
  MacTexture *texture =
      static_cast<MacTexture *>(std::malloc(sizeof(MacTexture)));
  if (texture == nullptr) {
    std::perror("malloc failed (createTexture)");
    return nullptr;
  }

  texture = new (texture) MacTexture(device, width, height);
  texture->addRef();

  @autoreleasepool {
    MTLTextureDescriptor *texture_desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:width
                                    height:height
                                 mipmapped:NO];

    // texture_desc.storageMode
    if (drawable_flag == TextureDrawable::Enable) {
      texture_desc.usage =
          MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    } else {
      texture_desc.usage = MTLTextureUsageShaderRead;
    }

    texture->data.texture = [device->getPlatformData().device
        newTextureWithDescriptor:texture_desc];
    if (texture->data.texture == nil) {
      texture->release();
      return nullptr;
    }
  }

  return texture;
}

MacTexture::MacTexture(ImpGraphicsDevice *device, int width, int height) {
  this->data.device = device;
  device->addRef();

  this->data.width = width;
  this->data.height = height;
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
