#pragma once
#include "../TextureTemplate.hpp"
#import "Metal/Metal.h"

//  ========================================================
//
//  Texture
//
//  ========================================================

struct TextureData {
  id<MTLTexture> mtl_texture = nil;
};

using Texture = TextureTemplate<TextureData>;

class MacTexture : public Texture {
private:
  MacTexture(IGraphicsDevice *, int, int, TextureFormat);
  // MacTexture(ImpGraphicsDevice *, int, int);

public:
  static MacTexture *createMacTexture(IGraphicsDevice *, int, int,
                                      const TextureDesc);
};
