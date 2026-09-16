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
//  Mac Texture Surface
//
//  ========================================================

MacTextureSurface::MacTextureSurface(IGraphicsDevice *device, ITexture *texture)
    : TextureSurface(device, texture) {}

// テクスチャ専用デストラクタ
MacTextureSurface::~MacTextureSurface() {
  // @autoreleasepool {
  //   // ~CommonSurface に任せたので不要になった
  //   if (this->texture != nullptr) {
  //     this->texture->release();
  //   }
  // }
}

MacTextureSurface *
MacTextureSurface::createMacSurfaceFromTexture(IGraphicsDevice *device,
                                               ITexture *texture) {
  if (texture == nullptr) {
    return nullptr;
  }
  MacTextureSurface *surface =
      static_cast<MacTextureSurface *>(std::malloc(sizeof(MacTextureSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createTextureSurface)");
    return nullptr;
  }

  surface = new (surface) MacTextureSurface(device, texture);
  // コンストラクタに任せるようにした
  // surface->addRef();

  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  // surface->device = static_cast<MacGraphicsDevice *>(device);
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  // surface->device->addRef();

  // 参照カウントを増やす
  // surface->texture = texture;
  // texture->addRef();

  @autoreleasepool {
    // Texture は 1 枚の描画先なので処理中か否かの二パターンの状態がある
    surface->data.in_flight_semaphore = dispatch_semaphore_create(2);
    if (surface->data.in_flight_semaphore == nil) {
      device->release();
    }
  }

  return surface;
}

//  ========================================================
//
//  Texture Surface Template
//
//  ========================================================

template <>
bool TextureSurface::render(RenderCallBack callback, void *data,
                            const RenderPassDesc pass_desc) {
  bool result = false;
  @autoreleasepool {
    // チケットを消費
    // 残っていればスルー
    // 残っていなければ返却されるまで停止
    if (dispatch_semaphore_wait(this->data.in_flight_semaphore,
                                pass_desc.frame_dropping ==
                                        FrameDropping::Enable
                                    ? DISPATCH_TIME_NOW
                                    : DISPATCH_TIME_FOREVER) != 0) {
      return false;
    }

    MacTexture *texture = static_cast<MacTexture *>(this->texture);
    MacGraphicsDevice *device = static_cast<MacGraphicsDevice *>(this->device);
    if (texture == nullptr || device == nullptr) {
      return false;
    }

    id<MTLTexture> mtl_texture = texture->getPlatformData().mtl_texture;

    float true_width = texture->getWidth();
    float true_height = texture->getHeight();

    if (mtl_texture == nil || true_width == 0 || true_height == 0) {
      return false;
    }

    RenderHelper helper(device);
    if (!helper.isReady()) {
      return false;
    }

    MTLRenderPassDescriptor *mtl_pass_desc =
        helper.getMTLRenderPassDescripter(mtl_texture, &pass_desc);
    if (mtl_pass_desc == nil) {
      return false;
    }

    // begin
    id<MTLCommandBuffer> cmd_buffer =
        [device->getPlatformData().command_queue commandBuffer];
    if (cmd_buffer == nil) {
      return false;
    }
    id<MTLRenderCommandEncoder> encoder =
        [cmd_buffer renderCommandEncoderWithDescriptor:mtl_pass_desc];
    if (encoder == nil) {
      return false;
    }

    result =
        helper.renderBase(encoder, true_width, true_height, callback, data);

    // end
    [encoder endEncoding];

    // ローカル変数にコピーしないと this がキャプチャされる
    // キャプチャしたものはヒープにコピーされて retain される
    dispatch_semaphore_t semaphore = this->data.in_flight_semaphore;
    [cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
      dispatch_semaphore_signal(semaphore); // 返却
    }];

    [cmd_buffer commit];

    return result;
  }
}
