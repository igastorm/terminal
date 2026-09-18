#pragma once
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

template <class PlatformData>
class RenderPassTemplate : public CommonRenderPass {
protected:
  PlatformData data;

  bool drawVertices(const Vertex *vertices, int vertex_count) override;
  bool drawVerticesTex(ITexture *, const VertexTex *vertices,
                       int vertex_count) override;

public:
  ~RenderPassTemplate() = default;
};