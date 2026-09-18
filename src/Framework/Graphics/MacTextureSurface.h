#pragma once

#include "MacSurface.h"

//  ========================================================
//
//  Texture Surface
//
//  ========================================================

struct TextureSurfaceData : public SurfaceData {
  // 今のところ空
};

using TextureSurface = SurfaceTemplate<TextureSurfaceData>;

class MacTextureSurface : public TextureSurface {
private:
  MacTextureSurface(IGraphicsDevice *, ITexture *);

public:
  MacTextureSurface() = delete;
  ~MacTextureSurface();
  static MacTextureSurface *createMacSurfaceFromTexture(IGraphicsDevice *,
                                                        ITexture *);
};