#pragma once
#include "../CommonGraphics.hpp"
#import "Metal/Metal.h"

//  ========================================================
//
//  Texture
//
//  ========================================================

struct TextureData {
  id<MTLTexture> mtl_texture = nil;
};

class MacTexture : public CommonTexture {
private:
TextureData data = {};  
  MacTexture(IGraphicsDevice *, int, int, TextureFormat);
  bool upload(const void *, size_t, size_t, const TextureDataRegion) override;

public:
  ~MacTexture();
  MacTexture() = delete;
  static MacTexture *createMacTexture(IGraphicsDevice *, int, int,
                                      const TextureDesc);
  [[nodiscard]] TextureData getPlatformData() const { return this->data; }
};
