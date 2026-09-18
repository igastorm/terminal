#pragma once

#include "RenderPassTemplate.hpp"
#import <Metal/Metal.h>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

struct RenderPassData {
  IGraphicsDevice *device = nullptr;
  id<MTLRenderCommandEncoder> encoder = nil;
  // id<MTLRenderPipelineState> pipeline_state = nil;
  // id<MTLRenderPipelineState> pipeline_state_tex = nil;
  // id<MTLRenderPipelineState> pipeline_state_tex_outline = nil;
  // id<MTLSamplerState> sampler_state = nil;
  bool is_ready = false;
  // id<MTLBuffer> vertex_buffer = nil;
};

using RenderPass = RenderPassTemplate<RenderPassData>;

class MacRenderPass : public RenderPass {
public:
  MacRenderPass(IGraphicsDevice *,
                id<MTLRenderCommandEncoder> /*, id<MTLBuffer>*/);
  bool isReady() const;
  ~MacRenderPass();
};
