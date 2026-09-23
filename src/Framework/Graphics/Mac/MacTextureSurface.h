#pragma once
#include "MacSurface.h"

//  ========================================================
//
//  Texture Surface
//
//  ========================================================

struct TextureSurfaceData : public SurfaceData {
  struct {
    float width;
    float r_height;
  } viewport = {};
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