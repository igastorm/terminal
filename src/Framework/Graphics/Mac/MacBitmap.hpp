#pragma once
#include "../CommonGraphics.hpp"
#include "IFont.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct FontRasterizerData {
  CGContextRef cg_context = nullptr;
};

class MacFontRasterizer : public CommonFontRasterizer {
private:
  FontRasterizerData data = {};
  MacFontRasterizer(CGContextRef);

public:
  static MacFontRasterizer *createMacFontRasterizer(IFont *, std::uint8_t *,
                                                    std::size_t, std::size_t,
                                                    std::size_t, std::size_t);
  MacFontRasterizer() = delete;
  ~MacFontRasterizer();
};
