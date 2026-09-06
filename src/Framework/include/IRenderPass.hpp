#pragma once
#include "IObject.hpp"
#include <cstdint>

struct RenderPassDesc {
  bool clear = true;
  std::uint32_t color = 0xFF000000;
};

struct Vertex {
  float position[2];
  std::uint32_t color;
};

class IRenderPass : private IObject {
public:
  virtual ~IRenderPass() = default;
  virtual bool drawVertices(const Vertex *, int) = 0;
  
  static inline const RenderPassDesc DEFAULT_DESC;
};

using RenderCallBack = void (*)(IRenderPass *, void *);
