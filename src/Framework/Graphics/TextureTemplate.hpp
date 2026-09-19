#pragma once
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Texture
//
//  ========================================================

template <class PlatformData> class TextureTemplate : public CommonTexture {
protected:
  PlatformData data;

  TextureTemplate(IGraphicsDevice *device, int w, int h, TextureFormat format)
      : CommonTexture(device, w, h, format) {};

public:
  TextureTemplate() = delete;
  ~TextureTemplate() = default;
  [[nodiscard]] PlatformData getPlatformData() const { return this->data; }
  bool upload(const void *, size_t, size_t, const TextureDataRegion) override;
};
