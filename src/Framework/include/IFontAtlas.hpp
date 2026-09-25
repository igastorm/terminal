#pragma once
#include "IObject.hpp"
#include "IRenderPass.hpp"
#include <cstdint>

struct GlyphUV {
  float u_min = 0.0f, v_min = 0.0f;
  float u_max = 0.0f, v_max = 0.0f;
};

class IFontAtlas : public IObject {
public:
  virtual bool drawText(IRenderPass *, const char *, float, float,
                        std::uint32_t) = 0;
  [[nodiscard]] virtual ITexture *getTexture() = 0;
  [[nodiscard]] virtual GlyphUV getGlyphUV(wchar_t) = 0;
  virtual ~IFontAtlas() = default;
};
