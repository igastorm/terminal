#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ----------------------------
//  Render Pass
//  ----------------------------

struct ImpRenderPassData {};

// using ImpMacRenderPass = ImpRenderPass<ImpRenderPassData,
// ImpApplicationData>;

//  ----------------------------
//  Graphics Device
//  ----------------------------

struct ImpGraphicsDeviceData {
  id<MTLDevice> device = nil;
  id<MTLCommandQueue> command_queue = nil;
  // ImpMacRenderPass* render_pass = nullptr;
};

using ImpMacGraphicsDevice =
    ImpGraphicsDevice<ImpGraphicsDeviceData, ImpApplicationData>;

template <>
ImpGraphicsDeviceData ImpMacGraphicsDevice::getPlatformData(void) const;

//  ----------------------------
//  Surface
//  ----------------------------

struct ImpSurfaceData {
  IWindow *window = nullptr;
  CAMetalLayer *layer = nil;
  ImpMacGraphicsDevice *device = nullptr;
  int width = 0;
  int height = 0;
};

using ImpSurface = ImpSurfaceTemplate<ImpSurfaceData, ImpApplicationData>;
