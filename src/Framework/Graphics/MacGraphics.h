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
  dispatch_semaphore_t in_flight_semaphore = nil;
  MacGraphicsDevice *device = nullptr;
};

using ImpSurface = ImpSurfaceTemplate<ImpSurfaceData>;

class MacWindowSurface : public ImpSurface {
private:
  IWindow *window = nullptr;
  CAMetalLayer *metal_layer = nil;
  
  bool render(RenderCallBack, void *, const RenderPassDesc) override;

public:
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(MacGraphicsDevice *,
                                                      IWindow *);
};

class MacTextureSurface : public ImpSurface {
private:
  ITexture *texture = nullptr;

  bool render(RenderCallBack, void *, const RenderPassDesc) override;

public:
~MacTextureSurface();
static MacTextureSurface *createMacSurfaceFromTexture(MacGraphicsDevice *,
                                                    ITexture *);
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
