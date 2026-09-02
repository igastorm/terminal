#pragma once
#include "IObject.hpp"
#include <cstdint>

class IRendererPass : public IObject {
public:
  virtual ~IRendererPass() = default;

  virtual void clear(std::uint32_t) = 0;
};

using RenderCallBack = void (*)(IRendererPass *, void *);
