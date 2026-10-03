#pragma once
#include "MacSurface.h"
#import <QuartzCore/QuartzCore.h>

//  ========================================================
//
//  Window Surface
//
//  ========================================================

struct WindowSurfaceData {
  CAMetalLayer *metal_layer = nil;
};

class MacWindowSurface : public MacSurface {
private:
  WindowSurfaceData data = {};
  MacWindowSurface(IGraphicsDevice *, IWindow *);

  bool render(RenderCallBack, void *, const RenderPassDesc) override;

public:
  MacWindowSurface() = delete;
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(IGraphicsDevice *,
                                                      IWindow *);
};
