#pragma once

#include "MacSurface.h"
#import <QuartzCore/QuartzCore.h>

//  ========================================================
//
//  Window Surface
//
//  ========================================================

struct WindowSurfaceData : public SurfaceData {
  CAMetalLayer *metal_layer = nil;
};

using WindowSurface = SurfaceTemplate<WindowSurfaceData>;

class MacWindowSurface : public WindowSurface {
private:
  MacWindowSurface(IGraphicsDevice *, IWindow *);

public:
  MacWindowSurface() = delete;
  ~MacWindowSurface();
  static MacWindowSurface *createMacSurfaceFromWindow(IGraphicsDevice *,
                                                      IWindow *);
};
