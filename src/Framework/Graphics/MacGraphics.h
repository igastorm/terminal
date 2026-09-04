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
//  Graphics Device
//  ----------------------------

struct ImpGraphicsDeviceData {
  id<MTLDevice> device = nil;
  id<MTLCommandQueue> command_queue = nil;
};

using ImpMacGraphicsDevice =
    ImpGraphicsDevice<ImpGraphicsDeviceData, ImpApplicationData>;

//  ----------------------------
//  Surface
//  ----------------------------

struct ImpSurfaceData {
  ImpMacGraphicsDevice* device = nullptr;
  int width = 0;
  int height = 0;
};

using ImpSurface = ImpSurfaceTemplate<ImpSurfaceData, ImpApplicationData>;