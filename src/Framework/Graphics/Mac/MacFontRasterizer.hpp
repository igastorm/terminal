#pragma once
#include "../FontRasterizerTemplate.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct FontRasterizerData {};

using FontRasterizer = FontRasterizerTemplate<FontRasterizerData>;

class MacFontRasterizer : public FontRasterizer {
private:
  MacFontRasterizer(IFont *);

public:
  static MacFontRasterizer *createMacFontRasterizer(IFont *);
  MacFontRasterizer() = delete;
  ~MacFontRasterizer();
};
