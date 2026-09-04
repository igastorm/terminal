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
//  Surface
//  ----------------------------

struct ImpSurfaceData {
  id<MTLDevice> device = nil;
  int width = 0;
  int height = 0;
};

using ImpSurface = ImpSurfaceTemplate<ImpSurfaceData>;

// プラットフォーム依存内部用クラス
// コンストラクタで MTLDeivce を渡すために経由する
class ImpMacSurface : public ImpSurface {
public:
  ImpMacSurface(id<MTLDevice>, int, int);
};

// 中身のオブジェクトの参照も増やす必要がある
template <> int ImpSurface::addRef() {
  if (this->data.device != nil) {
    [this->data.device retain];
  }
  return this->addRefBase();
}

template <> bool ImpSurface::bindToWindow(IWindow *window) {
  window->addRef();
  return true;
}

template <>
void ImpSurface::unbindWindow() { // ウィンドウの参照カウントを減らす
}

template <> ImpSurface::~ImpSurfaceTemplate<ImpSurfaceData>() {
  @autoreleasepool {
    this->unbindWindow();
    if (this->data.device != nil) {
      // 参照カウントを減らす
      [this->data.device release];
      this->data.device = nil;
    }
  }
}

ImpMacSurface::ImpMacSurface(id<MTLDevice> mtl_device, int w, int h) {
  // 参照カウントを増やす
  this->data.device = [mtl_device retain];
  this->data.width = w;
  this->data.height = h;
}

//  ----------------------------
//  Graphics Device
//  ----------------------------

struct ImpGraphicsDeviceData {
  id<MTLDevice> device = nil;
  id<MTLCommandQueue> command_queue = nil;
};

using ImpMacGraphicsDevice = ImpGraphicsDevice<ImpGraphicsDeviceData>;

// 中身のオブジェクトの参照も増やす必要がある
template <> int ImpMacGraphicsDevice::addRef() {
  if (this->data.device != nil) {
    [this->data.device retain];
  }
  // 恐らく command_queue
  // は他のオブジェクトから参照することはないと思われるので retain しない
  /*
  if (this->data.command_queue != nil) {
    [this->data.command_queue retain];
  }
  */
  return this->addRefBase();
}

template <> ImpMacGraphicsDevice::~ImpGraphicsDevice<ImpGraphicsDeviceData>() {
  @autoreleasepool {
    if (this->data.command_queue != nil) {
      [this->data.command_queue release];
      this->data.command_queue = nil;
    }
    if (this->data.device != nil) {
      [this->data.device release];
      this->data.device = nil;
    }
  }
}

template <>
bool ImpMacGraphicsDevice::render(ISurface *, RenderCallBack, void *) {
  return true;
}

template <>
ISurface *ImpMacGraphicsDevice::createSurface(int width, int height) {
  @autoreleasepool {
    ImpMacSurface *surface =
        static_cast<ImpMacSurface *>(std::malloc(sizeof(ImpMacSurface)));
    if (surface == nullptr) {
      std::perror("malloc failed (createSurface)");
      return nullptr;
    }
    surface = new (surface) ImpMacSurface(this->data.device, width, height);
    // この関数で生成するので ref_count を加算するだけ
    surface->addRef();

    // surface は device を参照するので参照カウントを増やす
    // (MTLDevice だけでいい)
    // しかし surface のコンストラクタで retain しているのでここではいらない

    return surface;
  }
}

template <>
template <>
ImpMacGraphicsDevice *
ImpMacGraphicsDevice::createGraphicsDevice<ImpApplicationData>(
    ImpApplication<ImpApplicationData> *appInstance) {
  @autoreleasepool {
    ImpMacGraphicsDevice *device = static_cast<ImpMacGraphicsDevice *>(
        std::malloc(sizeof(ImpMacGraphicsDevice)));
    if (device == nullptr) {
      std::perror("malloc failed (createGraphicsDevice)");
      return nullptr;
    }

    device = new (device) ImpMacGraphicsDevice;
    // この関数で生成するので ref_count を加算するだけ
    device->addRef();

    if ((device->data.device = MTLCreateSystemDefaultDevice()) == nil) {
      device->release();
      return nullptr;
    }

    if ((device->data.command_queue = [device->data.device newCommandQueue]) ==
        nil) {
      device->release();
      return nullptr;
    }

    return device;
  }
}

template <>
IGraphicsDevice *ImpApplication<ImpApplicationData>::createGraphicsDevice() {
  ImpMacGraphicsDevice *device =
      ImpMacGraphicsDevice::createGraphicsDevice(this);
  return device;
}
