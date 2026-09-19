#pragma once
#include "IObject.hpp"
#include "IRenderPass.hpp"
#include <cstdint>

class IFontAtlas : public IObject {
public:
  virtual bool drawText(IRenderPass *, const char *, float, float,
                        std::uint32_t) = 0;
  virtual ~IFontAtlas() = default;
};
