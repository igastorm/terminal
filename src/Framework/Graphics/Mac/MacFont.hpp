#pragma once
#include "../CommonGraphics.hpp"
#include "IFont.hpp"
#include <CoreText/CTFont.h>

struct MacFontData {
  CTFontRef ct_font = nullptr;
  float size = 0.0f;
};

class MacFont : public CommonFont {
private:
  MacFontData data = {};
  MacFont(CTFontRef, float);

  FontCellSize getCellSize() const override;

  static CTFontRef createCTFont(const char8_t *, float);

public:
  [[nodiscard]] MacFontData getData() const;
  MacFont() = delete;
  ~MacFont();
  static MacFont *createFont(const char8_t *, float);
};
