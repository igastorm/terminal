#pragma once
#pragma once
#include "IFont.hpp"
#include "IObject.hpp"

class IFontRasterizer : public IObject {
public:
  ~IFontRasterizer() = default;
  [[nodiscard]]
  static IFontRasterizer *createFontRasterizer(IFont *);
};
