#pragma once
#include "CommonGraphics.hpp"
#include "IFont.hpp"

template <class PlatformData> class FontRasterizerTemplate : public CommonFontRasterizer {
protected:
  PlatformData data;

  FontRasterizerTemplate(IFont *font) : CommonFontRasterizer(font) {};

public:
  FontRasterizerTemplate() = delete;
  ~FontRasterizerTemplate() = default;
};
