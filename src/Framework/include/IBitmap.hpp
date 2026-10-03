#pragma once
#pragma once
#include "IObject.hpp"
#include <cstddef>

class IBitmap : public IObject {
public:
  ~IBitmap() = default;
  [[nodiscard]]
  static IBitmap *createBitmap(std::size_t, std::size_t, std::size_t,
                               std::size_t);
};
