#pragma once
#include "../Font.hpp"
#include <CoreText/CTFont.h>

struct MacFontData {
  CTFontRef ct_font = nullptr;
};

class MacFont : public Font {
private:
  MacFontData data = {};
  MacFont(CTFontRef, float);

  bool drawGlyph(IBitmap *, const char32_t, int, int) override;

  FontCellSize getCellSize() const override;

  static CTFontRef createCTFont(const char8_t *, float);

public:
  MacFont() = delete;
  ~MacFont();
  static MacFont *createMacFont(const char8_t *, float);
};
