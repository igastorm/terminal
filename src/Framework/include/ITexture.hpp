#pragma once
#include "IObject.hpp"
#include <cstddef>

// Enable にすると Render To Texture ができるが重くなるらしい
enum class TextureDrawable { Enable, Disable };

class ITexture : public IObject {
public:
  virtual ~ITexture() = default;

  virtual bool upload(const void *, size_t, size_t) = 0;
};
