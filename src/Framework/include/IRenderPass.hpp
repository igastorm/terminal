#pragma once
#include "IObject.hpp"
#include <cstdint>

struct RenderPassDesc {
  bool clear = true;
  std::uint32_t color = 0xFF000000;
};

class IRenderPass : private IObject {
public:
  virtual ~IRenderPass() = default;
  static inline const RenderPassDesc DEFAULT_DESC;
};

using RenderCallBack = void (*)(IRenderPass *, void *);
