#pragma once
#include "IObject.hpp"
#include <cstddef>

// Enable にすると Render To Texture ができるが重くなるらしい
enum class TextureDrawable { Enable, Disable };

enum class TextureFormat {
  // 画像用 (カラー)
  Color,

  // フォント用 (白黒)
  Mono
};

struct TextureDesc {
  TextureDrawable drawable_flag = TextureDrawable::Disable;
  TextureFormat format = TextureFormat::Color;
};

class ITexture : public IObject {
public:
  virtual ~ITexture() = default;

  virtual bool upload(const void *, size_t, size_t) = 0;

  static inline constexpr TextureDesc DEFAULT_DESC;
};
