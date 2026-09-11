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
  id<MTLRenderPipelineState> pipeline_state = nil;
  id<MTLRenderPipelineState> pipeline_state_tex = nil;
  id<MTLRenderPipelineState> pipeline_state_tex_outline = nil;
  id<MTLSamplerState> sampler_state = nil;
  bool is_ready = false;
  // id<MTLBuffer> vertex_buffer = nil;
};

using ImpRenderPass = ImpRenderPassTemplate<ImpRenderPassData>;

class MacRenderPass : public ImpRenderPass {
private:
public:
  MacRenderPass(id<MTLRenderCommandEncoder>, id<MTLRenderPipelineState>,
                id<MTLRenderPipelineState>, id<MTLRenderPipelineState>,
                id<MTLSamplerState> /*, id<MTLBuffer>*/);
  bool isReady() const;
  ~MacRenderPass();
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
  id<MTLRenderPipelineState> pipeline_state_tex_outline = nil;
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
  static MacGraphicsDevice *createMacGraphicsDevice(ImpApplication *);
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

struct ImpWindowSurfaceData : public ImpSurfaceData {
  IWindow *window = nullptr;
  CAMetalLayer *metal_layer = nil;
};

struct ImpTextureSurfaceData : public ImpSurfaceData {
  ITexture *texture = nullptr;
};

using ImpWindowSurface = ImpSurfaceTemplate<ImpWindowSurfaceData>;
using ImpTextureSurface = ImpSurfaceTemplate<ImpTextureSurfaceData>;

class RenderHelper {
private:
  static constexpr float inv_255 = 1.0f / 255.0f;
  MTLRenderPassDescriptor *mtl_pass_desc = nil;

public:
  MTLRenderPassDescriptor *getMTLRenderPassDescripter(id<MTLTexture>,
                                                      const RenderPassDesc *);
  void renderBase(id<MTLRenderCommandEncoder>, id<MTLRenderPipelineState>,
                  id<MTLRenderPipelineState>, id<MTLRenderPipelineState>,
                  id<MTLSamplerState>, float, float, RenderCallBack callback,
                  void *data);
  RenderHelper(MacGraphicsDevice *);
  ~RenderHelper();
};

class MacWindowSurface : public ImpWindowSurface {
public:
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(ImpGraphicsDevice *,
                                                      IWindow *);
};

class MacTextureSurface : public ImpTextureSurface {
public:
  ~MacTextureSurface();
  static MacTextureSurface *createMacSurfaceFromTexture(ImpGraphicsDevice *,
                                                        ITexture *);
};

//  ========================================================
//
//  Texture
//
//  ========================================================

struct ImpTextureData {
  id<MTLTexture> texture = nil;
  MacGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;
};

using ImpTexture = ImpTextureTemplate<ImpTextureData>;

class MacTexture : public ImpTexture {
private:
  MacTexture() = default;
  // MacTexture(ImpGraphicsDevice *, int, int);

public:
  static MacTexture *createMacTexture(ImpGraphicsDevice *, int, int,
                                      const TextureDesc);
};
