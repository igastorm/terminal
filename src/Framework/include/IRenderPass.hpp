#pragma once
#include "IObject.hpp"
#include <cstdint>

enum class FrameDropping {
  Enable,
  Disable
};

struct RenderPassDesc {
  bool clear = true;
  std::uint32_t color = 0xFF000000;
  FrameDropping frame_dropping = FrameDropping::Disable;
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
