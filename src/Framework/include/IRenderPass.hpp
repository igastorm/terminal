#pragma once
#include "IObject.hpp"
#include <cstdint>

class IRenderPass : public IObject {
public:
  virtual ~IRenderPass() = default;

  virtual void clear(std::uint32_t) = 0;
};

using RenderCallBack = void (*)(IRenderPass *, void *);
