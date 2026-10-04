#pragma once
#include "../Application/Mac/MacApplication.h"
#include "../CommonGraphics.hpp"
#import <Metal/Metal.h>

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

struct GraphicsDeviceData {
  id<MTLDevice> mtl_device = nil;
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

class MacGraphicsDevice : public CommonGraphicsDevice {
private:
  GraphicsDeviceData data;

  MacGraphicsDevice(IApplication *);

  ISurface *createSurfaceFromWindow(IWindow *) override;
  ISurface *createSurfaceFromTexture(ITexture *) override;
  ITexture *createTexture(int, int, const TextureDesc) override;
  IFontAtlas *createFontAtlas(const char *, float, int, int) override;

public:
  static MacGraphicsDevice *createMacGraphicsDevice(IApplication *);
  
  [[nodiscard]] GraphicsDeviceData getPlatformData() const {
    return this->data;
  }
  
  MacGraphicsDevice() = delete;
  ~MacGraphicsDevice();
};
