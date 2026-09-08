#pragma once
#include "IObject.hpp"

// Enable にすると Render To Texture ができるが重くなるらしい
enum class TextureDrawable { Enable, Disable };

class ITexture : public IObject {
public:
  virtual ~ITexture() = default;
};
