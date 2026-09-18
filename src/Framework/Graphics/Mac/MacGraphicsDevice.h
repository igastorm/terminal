#pragma once
#include "../Application/Mac/MacApplication.h"
#include "../GraphicsDeviceTemplate.hpp"
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

using GraphicsDevice =
    GraphicsDeviceTemplate<GraphicsDeviceData, ApplicationData>;

class MacGraphicsDevice : public GraphicsDevice {
private:
  MacGraphicsDevice(IApplication *);

public:
  static MacGraphicsDevice *createMacGraphicsDevice(IApplication *);
  MacGraphicsDevice() = delete;
  ~MacGraphicsDevice();
};
