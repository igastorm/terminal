#include "MacGraphics.h"
#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Surface
//
//  ========================================================

MacSurface::MacSurface(ImpGraphicsDevice *device) {
  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  this->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  this->data.device->addRef();
}

MacSurface *MacSurface::createMacSurface(ImpGraphicsDevice *device) {
  MacSurface *surface =
      static_cast<MacSurface *>(std::malloc(sizeof(MacSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createSurface)");
    return nullptr;
  }

  surface = new (surface) MacSurface(device);
  surface->addRef();

  return surface;
}

template <> void ImpSurface::unbindWindow() {
  this->bind_flag = BindObject::none;
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
  this->bind_flag = BindObject::window;

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
    // フレームの絵が引き伸ばされずに, その場に元の等倍サイズのまま留まる
    // ウィンドウサイズを変更した時にブレるのを防止
    layer.contentsGravity = kCAGravityTopLeft;
    // なんか OS 側のアニメーションのタイミングを調整するらしい
    // layer.presentsWithTransaction = YES;
    if (view.window == nil) {
      // 一応ガード用 if があるがこれが nil ということは createWindow がおかしい
      [layer release];
      this->unbindWindow();
      return false;
    }

    CGFloat scale = [view.window backingScaleFactor];
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(view.bounds.size.width * scale,
                                    view.bounds.size.height * scale);

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

template <> ImpSurface::~ImpSurfaceTemplate<ImpSurfaceData>() {
  @autoreleasepool {
    this->unbindWindow();
    if (this->data.device != nullptr) {
      // 参照カウントを減らす
      this->data.device->release();
      this->data.device = nullptr;
    }
    // unbind の方でやるから不要 Layer についても
    /*
    if (this->data.window != nullptr) {
      this->data.window->release();
      this->data.window = nullptr;
    }
    */
  }
}
