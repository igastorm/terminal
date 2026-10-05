#pragma once
#include "IObject.hpp"
#include <cstddef>

class IBitmap : public IObject {
public:
  ~IBitmap() = default;
  [[nodiscard]] virtual std::size_t getWidth() const = 0;
  [[nodiscard]] virtual std::size_t getHeight() const = 0;
  [[nodiscard]] virtual void* getBitmapData() const = 0;
  [[nodiscard]]
  static IBitmap *createBitmap(::size_t, std::size_t);
};
