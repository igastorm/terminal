#pragma once
#include "../CommonGraphics.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct FontRasterizerData {};

class MacFontRasterizer : public CommonFontRasterizer {
private:
  MacFontRasterizer(IFont *);

public:
  static MacFontRasterizer *createMacFontRasterizer(IFont *);
  MacFontRasterizer() = delete;
  ~MacFontRasterizer();
};
