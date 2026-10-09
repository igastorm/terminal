#pragma once
#include "../RenderPass.hpp"
#include "IGraphicsDevice.hpp"
#import <Metal/Metal.h>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

struct RenderPassData {
  id<MTLRenderCommandEncoder> encoder = nil;
  // id<MTLRenderPipelineState> pipeline_state = nil;
  // id<MTLRenderPipelineState> pipeline_state_tex = nil;
  // id<MTLRenderPipelineState> pipeline_state_tex_outline = nil;
  // id<MTLSamplerState> sampler_state = nil;
  bool is_ready = false;
  // id<MTLBuffer> vertex_buffer = nil;
};

class MacRenderPass : public RenderPass {
private:
  RenderPassData data = {};
  bool drawVertices(const Vertex *vertices, int vertex_count) override;
  bool drawVerticesTex(ITexture *, const VertexTex *vertices,
                       int vertex_count) override;

public:
  MacRenderPass(IGraphicsDevice *,
                id<MTLRenderCommandEncoder> /*, id<MTLBuffer>*/);
  bool isReady() const;
  ~MacRenderPass();
};
