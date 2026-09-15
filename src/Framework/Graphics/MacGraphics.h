#pragma once
#include "../Application/ApplicationTemplate.hpp"
#include "../Application/MacApplication.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "GraphicsTemplate.hpp"
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
private:
public:
  static MacGraphicsDevice *createMacGraphicsDevice(Application *);
};

template <>
GraphicsDeviceData GraphicsDevice::getPlatformData(void) const;

//  ========================================================
//
//  Surface
//
//  ========================================================

struct SurfaceData {
  dispatch_semaphore_t in_flight_semaphore = nil;
  MacGraphicsDevice *device = nullptr;
};

struct WindowSurfaceData : public SurfaceData {
  IWindow *window = nullptr;
  CAMetalLayer *metal_layer = nil;
};

struct TextureSurfaceData : public SurfaceData {
  ITexture *texture = nullptr;
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
public:
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(GraphicsDevice *,
                                                      IWindow *);
};

class MacTextureSurface : public TextureSurface {
public:
  ~MacTextureSurface();
  static MacTextureSurface *createMacSurfaceFromTexture(GraphicsDevice *,
                                                        ITexture *);
};

//  ========================================================
//
//  Texture
//
//  ========================================================

struct TextureData {
  id<MTLTexture> texture = nil;
  MacGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;
};

using Texture = TextureTemplate<TextureData>;

class MacTexture : public Texture {
private:
  MacTexture() = default;
  // MacTexture(ImpGraphicsDevice *, int, int);

public:
  static MacTexture *createMacTexture(GraphicsDevice *, int, int,
                                      const TextureDesc);
};
