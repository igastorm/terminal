#include "MacGraphics.h"
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

// プラットフォーム依存内部用クラス
// コンストラクタで MTLDeivce を渡すために経由する
// 渡し忘れを防ぐため, ImpSurface のコンストラクタが private になっている
class ImpMacSurface : public ImpSurface {
public:
  ImpMacSurface(ImpApplication<ImpApplicationData> *, ImpMacGraphicsDevice *,
                int, int);
};

template <> bool ImpSurface::bindToWindow(IWindow *window) {
  window->addRef();
  return true;
}

template <>
void ImpSurface::unbindWindow() { // ウィンドウの参照カウントを減らす
}

template <>
ImpSurface::~ImpSurfaceTemplate<ImpSurfaceData, ImpApplicationData>() {
  @autoreleasepool {
    this->unbindWindow();
    if (this->data.device != nullptr) {
      // 参照カウントを減らす
      this->data.device->release();
      this->data.device = nullptr;
    }
  }
}

ImpMacSurface::ImpMacSurface(ImpApplication<ImpApplicationData> *appInstance,
                             ImpMacGraphicsDevice *device, int w, int h) {
  // device を参照
  this->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  this->data.device->addRef();

  this->data.width = w;
  this->data.height = h;
}

//  ----------------------------
//  Graphics Device
//  ----------------------------

template <>
ImpMacGraphicsDevice::~ImpGraphicsDevice<ImpGraphicsDeviceData,
                                         ImpApplicationData>() {
  @autoreleasepool {
    if (this->data.command_queue != nil) {
      [this->data.command_queue release];
      this->data.command_queue = nil;
    }
    if (this->data.device != nil) {
      [this->data.device release];
      this->data.device = nil;
    }
    if (this->appInstance != nullptr) {
      this->appInstance->release();
      this->appInstance = nullptr;
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

    surface =
        new (surface) ImpMacSurface(this->appInstance, this, width, height);
    surface->addRef();

    return surface;
  }
}

template <>
ImpMacGraphicsDevice *ImpMacGraphicsDevice::createGraphicsDevice(
    ImpApplication<ImpApplicationData> *appInstance) {
  @autoreleasepool {
    ImpMacGraphicsDevice *device = static_cast<ImpMacGraphicsDevice *>(
        std::malloc(sizeof(ImpMacGraphicsDevice)));
    if (device == nullptr) {
      std::perror("malloc failed (createGraphicsDevice)");
      return nullptr;
    }

    device = new (device) ImpMacGraphicsDevice;
    device->addRef();

    // appInstance を参照
    device->appInstance = appInstance;
    device->appInstance->addRef();

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
