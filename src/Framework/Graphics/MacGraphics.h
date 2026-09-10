#pragma once
#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
#include "shaders_metallib.hpp"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

struct ImpRenderPassData {
  id<MTLRenderCommandEncoder> encoder = nil;
  // id<MTLBuffer> vertex_buffer = nil;
};

using ImpRenderPass = ImpRenderPassTemplate<ImpRenderPassData>;

class MacRenderPass : public ImpRenderPass {
public:
  MacRenderPass(id<MTLRenderCommandEncoder> /*, id<MTLBuffer>*/);
};

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

struct ImpGraphicsDeviceData {
  id<MTLDevice> device = nil;
  id<MTLCommandQueue> command_queue = nil;
  id<MTLRenderPipelineState> pipeline_state = nil;
  id<MTLRenderPipelineState> pipeline_state_tex = nil;
  id<MTLSamplerState> sampler_state = nil;
  // id<MTLBuffer> vertex_buffer = nil;
  // セマフォはデバイスごとではなく描画先ごとに持つべきらしいので Surface
  // に引っ越し
  // dispatch_semaphore_t in_flight_semaphore = nil;
};

using ImpGraphicsDevice =
    ImpGraphicsDeviceTemplate<ImpGraphicsDeviceData, ImpApplicationData>;

class MacGraphicsDevice : public ImpGraphicsDevice {
private:
public:
  static MacGraphicsDevice *
  createMacGraphicsDevice(ImpApplication<ImpApplicationData> *);
};

template <>
ImpGraphicsDeviceData ImpGraphicsDevice::getPlatformData(void) const;

//  ========================================================
//
//  Surface
//
//  ========================================================

struct ImpSurfaceData {
  union {
    // ポインタサイズ分を共有することになる
    ITexture *texture = nullptr;
    IWindow *window;
  };
  dispatch_semaphore_t in_flight_semaphore = nil;

  // 描画先がテクスチャの時は無駄になってしまう
  // 実質フラグとして使ってるからいいか
  CAMetalLayer *layer = nil;

  MacGraphicsDevice *device = nullptr;
};

using ImpSurface = ImpSurfaceTemplate<ImpSurfaceData>;

// プラットフォーム依存内部用クラス
// コンストラクタで MTLDeivce を渡すために経由する
// 渡し忘れを防ぐため, ImpSurface のコンストラクタが protected になっている
class MacSurface : public ImpSurface {
private:
  MacSurface() = default;
  static MacSurface *createMacSurfaceBase(MacGraphicsDevice *);

public:
  static MacSurface *createMacSurfaceFromWindow(MacGraphicsDevice *, IWindow *);
};

//  ========================================================
//
//  Texture
//
//  ========================================================

struct ImpTextureData {
  id<MTLTexture> texture = nil;
  ImpGraphicsDevice *device = nullptr;
  int width = 0;
  int height = 0;
};

using ImpTexture = ImpTextureTemplate<ImpTextureData>;

class MacTexture : public ImpTexture {
private:
  MacTexture() = default;
  // MacTexture(ImpGraphicsDevice *, int, int);

public:
  static MacTexture *createMacTexture(MacGraphicsDevice *, int, int,
                                      TextureDrawable);
};
