#pragma once
#include "../Application/ApplicationTemplate.hpp"
#include "../Application/MacApplication.h"
#include "GraphicsTemplate.hpp"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
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

struct RenderPassData {
  id<MTLRenderCommandEncoder> encoder = nil;
  id<MTLRenderPipelineState> pipeline_state = nil;
  id<MTLRenderPipelineState> pipeline_state_tex = nil;
  id<MTLRenderPipelineState> pipeline_state_tex_outline = nil;
  id<MTLSamplerState> sampler_state = nil;
  bool is_ready = false;
  // id<MTLBuffer> vertex_buffer = nil;
};

using RenderPass = RenderPassTemplate<RenderPassData>;

class MacRenderPass : public RenderPass {
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

struct GraphicsDeviceData {
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

using GraphicsDevice =
    GraphicsDeviceTemplate<GraphicsDeviceData, ApplicationData>;

class MacGraphicsDevice : public GraphicsDevice {
public:
  static MacGraphicsDevice *createMacGraphicsDevice(Application *);
};

template <> GraphicsDeviceData GraphicsDevice::getPlatformData(void) const;

//  ========================================================
//
//  Surface
//
//  ========================================================

struct SurfaceData {
  dispatch_semaphore_t in_flight_semaphore = nil;
};

struct WindowSurfaceData : public SurfaceData {
  CAMetalLayer *metal_layer = nil;
};

struct TextureSurfaceData : public SurfaceData {
  // 今のところ空
};

using WindowSurface = SurfaceTemplate<WindowSurfaceData>;
using TextureSurface = SurfaceTemplate<TextureSurfaceData>;

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

class MacWindowSurface : public WindowSurface {
private:
  MacWindowSurface(IGraphicsDevice *, IWindow *);

public:
  MacWindowSurface() = delete;
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(IGraphicsDevice *,
                                                      IWindow *);
};

class MacTextureSurface : public TextureSurface {
private:
  MacTextureSurface(IGraphicsDevice *, ITexture *);

public:
  MacTextureSurface() = delete;
  ~MacTextureSurface();
  static MacTextureSurface *createMacSurfaceFromTexture(IGraphicsDevice *,
                                                        ITexture *);
};

//  ========================================================
//
//  Texture
//
//  ========================================================

struct TextureData {
  id<MTLTexture> texture = nil;
};

using Texture = TextureTemplate<TextureData>;

class MacTexture : public Texture {
private:
  MacTexture(IGraphicsDevice *, int, int, TextureFormat);
  // MacTexture(ImpGraphicsDevice *, int, int);

public:
  static MacTexture *createMacTexture(IGraphicsDevice *, int, int,
                                      const TextureDesc);
};
