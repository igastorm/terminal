#include "MacGraphics.h"
#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
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
class ImpMacRenderPass : public ImpRenderPass {
public:
  ImpMacRenderPass(id<MTLRenderCommandEncoder>);
};

ImpMacRenderPass::ImpMacRenderPass(id<MTLRenderCommandEncoder> encoder) {
  this->data.encoder = encoder;
}

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

template <> void ImpSurface::unbindWindow() {
  @autoreleasepool {
    if (this->data.window != nullptr) {
      static_cast<ImpMacWindow *>(this->data.window)
          ->getPlatformData()
          .view.layer = nil;

      static_cast<ImpMacWindow *>(this->data.window)
          ->getPlatformData()
          .view.wantsLayer = NO;

      this->data.window->release();
      this->data.window = nullptr;
    }
    if (this->data.layer != nil) {
      [this->data.layer release];
      this->data.layer = nil;
    }
  }
}

template <> bool ImpSurface::bindToWindow(IWindow *window) {
  this->unbindWindow();
  if (window == nullptr) {
    return false;
  }

  // 参照を増やす
  this->data.window = window;
  window->addRef();

  @autoreleasepool {
    // これ逆向き的なキャストだけどいいのか (やらないと無理そうだが) ← OK (適切)
    // getter を IWindow に追加すればいいがそれだと内部が漏れる
    // ImpMacWindow
    // の先頭にマジックナンバーを置いて逆キャストできるかチェックするのもできる
    WindowView *view =
        static_cast<ImpMacWindow *>(this->data.window)->getPlatformData().view;

    // view はポインタなので view に変更を加えると window
    // 側にも反映される
    // 型は NSView でも WindowView でもどっちでもいい
    if (view == nil) {
      // ここの条件も成り立ったらもはやバグ
      this->unbindWindow();
      return false;
    }

    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    // device の一部を参照するがデストラクタで必ず unbind を呼ぶので addRef
    // は不要
    layer.device = this->data.device->getPlatformData().device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    // NO にすると CPU で読み取りができるってことか
    // しかし重たくなると思われる
    // Unified メモリアーキテクチャだとそうでもない気がするが
    layer.framebufferOnly = YES;
    // ウィンドウ全体に貼り付ける
    layer.frame = view.bounds;
    if (view.window == nil) {
      // 一応ガード用 if があるがこれが nil ということは createWindow がおかしい
      // 高 DPI 対応 (Retina) ピクセルレベルのビューサイズを得るっぽい
      layer.contentsScale = [view.window backingScaleFactor];
      [layer release];
      this->unbindWindow();
      return false;
    }

    // Metal レイヤーを貼り付ける
    view.layer = layer;
    view.wantsLayer = YES;

    this->data.layer = layer;
  }
  return true;
}

template <> ImpSurfaceData ImpSurface::getPlatformData() const {
  return this->data;
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
    // unbind の方でやるから不要
    /*
    if (this->data.window != nullptr) {
      this->data.window->release();
      this->data.window = nullptr;
    }
    */
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
ImpGraphicsDeviceData ImpMacGraphicsDevice::getPlatformData() const {
  return this->data;
}

template <>
bool ImpMacGraphicsDevice::render(ISurface *isurface, RenderCallBack callback,
                                  void *data, const RenderPassDesc pass_desc) {
  if (isurface == nullptr || callback == nullptr) {
    return false;
  }

  @autoreleasepool {
    ImpSurface *surface = static_cast<ImpSurface *>(isurface);
    CAMetalLayer *layer = surface->getPlatformData().layer;

    if (layer == nil) {
      return false;
    }

    // バックバッファを取得
    // 複数のバッファがあって, 表示中のバッファ,
    // 描画中のバッファというようになってるらしい (ティアリング 防止)
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (drawable == nil) {
      return false;
    }

    MTLRenderPassDescriptor *desc =
        [MTLRenderPassDescriptor renderPassDescriptor];

    // 描き込み先のテクスチャ
    desc.colorAttachments[0].texture = drawable.texture;

    // 描画を開始時に前のフレームをどうするか
    // Clear: 指定色でクリア, Load: 保持 (遅いらしい)
    desc.colorAttachments[0].loadAction =
        pass_desc.clear ? MTLLoadActionClear : MTLLoadActionLoad;

    // 描画が終わった後, 結果をテクスチャに保存するか
    desc.colorAttachments[0].storeAction = MTLStoreActionStore;

    // クリアに使う色
    desc.colorAttachments[0].clearColor =
        MTLClearColorMake(((pass_desc.color >> 16) & 0xFF) / 255.0,
                          ((pass_desc.color >> 8) & 0xFF) / 255.0,
                          (pass_desc.color & 0xFF) / 255.0,
                          ((pass_desc.color >> 24) & 0xFF) / 255.0);

    // begin
    id<MTLCommandBuffer> cmdBuffer = [this->data.command_queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [cmdBuffer renderCommandEncoderWithDescriptor:desc];

    // ここでコールバック (beign-end)
    ImpMacRenderPass pass(encoder);
    [encoder retain];
    callback(&pass, data);
    [encoder release];

    // end
    [encoder endEncoding];
    [cmdBuffer presentDrawable:drawable];
    [cmdBuffer commit];

    return true;
  }
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
