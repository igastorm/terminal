#pragma once
#include "MacSurface.h"

//  ========================================================
//
//  Texture Surface
//
//  ========================================================

struct TextureSurfaceData {
  struct {
    float width;
    float r_height;
  } viewport = {};
};

class MacTextureSurface : public MacSurface {
private:
  TextureSurfaceData data = {};
  MacTextureSurface(IGraphicsDevice *, ITexture *);
  
  bool render(RenderCallBack, void *, const RenderPassDesc) override;

public:
  MacTextureSurface() = delete;
  ~MacTextureSurface();
  static MacTextureSurface *createMacSurfaceFromTexture(IGraphicsDevice *,
                                                        ITexture *);
};
