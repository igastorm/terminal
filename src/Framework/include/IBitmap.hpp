#pragma once
#pragma once
#include "IFont.hpp"
#include "IObject.hpp"
#include <cstddef>
#include <cstdint>

class IFontRasterizer : public IObject {
public:
  ~IFontRasterizer() = default;
  [[nodiscard]]
  static IFontRasterizer *createFontRasterizer(IFont *, std::uint8_t *,
                                               std::size_t, std::size_t,
                                               std::size_t, std::size_t);
};
