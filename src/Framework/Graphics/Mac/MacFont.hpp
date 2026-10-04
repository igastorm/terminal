#pragma once
#include "../Font.hpp"
#include <CoreText/CTFont.h>

struct MacFontData {
  CTFontRef ct_font = nullptr;
  CGFloat descent = 0.0f;
};

class MacFont : public Font {
private:
  MacFontData data = {};
  MacFont(CTFontRef, float, float, float);

  bool drawGlyph(IBitmap *, const char32_t, int, int) override;

  static CTFontRef createCTFont(const char8_t *, float);

public:
  MacFont() = delete;
  ~MacFont();
  static MacFont *createMacFont(const char8_t *, float);
};
