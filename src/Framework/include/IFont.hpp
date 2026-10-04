#pragma once
#include "IBitmap.hpp"
#include "IObject.hpp"

struct FontCellSize {
  float half_width = 0.0f;
  float height = 0.0f;
};

class IFont : public IObject {
public:
  virtual bool drawGlyph(IBitmap *, const char32_t, int, int) = 0;
  virtual FontCellSize getCellSize() const = 0;
  static IFont *createFont(const char8_t *, float);
  virtual ~IFont() = default;
};
