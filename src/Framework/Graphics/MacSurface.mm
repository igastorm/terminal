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

MacSurface *MacSurface::createMacSurfaceBase(MacGraphicsDevice * device) {
  MacSurface *surface =
      static_cast<MacSurface *>(std::malloc(sizeof(MacSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createSurface)");
    return nullptr;
  }

  surface = new (surface) MacSurface;
  surface->addRef();

  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  surface->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  surface->data.device->addRef();

  return surface;
}

MacSurface *MacSurface::createMacSurfaceFromWindow(MacGraphicsDevice *device, IWindow* window) {
  MacSurface *surface = MacSurface::createMacSurfaceBase(device);
  if (surface == nullptr || window == nullptr) {
    surface->release();
    return nullptr;
  }

  // 参照カウントを増やす
  surface->data.window = window;
  window->addRef();

  @autoreleasepool {
    // getter を IWindow に追加すればいいがそれだと内部が漏れる
    WindowView *view =
        static_cast<ImpMacWindow *>(surface->data.window)->getPlatformData().view;

    // view はポインタなので view に変更を加えると window
    // 側にも反映される
    // 型は NSView でも WindowView でもどっちでもいい
    if (view == nil) {
      surface->release();
      return nullptr;
    }

    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    // createMacSurface ですでに device の参照カウントを増やしてある
    layer.device = surface->data.device->getPlatformData().device;
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
    // 親レイヤーのサイズ変更を追従する
    // これがなくても手動で横方向は OK だが縦方向が遅延する
    // どうやら原点が左下らしいからそれが原因
    layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
    // なんか OS 側のアニメーションのタイミングを調整するらしい
    // layer.presentsWithTransaction = YES;
    if (view.window == nil) {
      // 一応ガード用 if があるがこれが nil ということは createWindow がおかしい
      [layer release];
      surface->release();
      return nullptr;
    }

    CGFloat scale = [view.window backingScaleFactor];
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(view.bounds.size.width * scale,
                                    view.bounds.size.height * scale);

    // Metal レイヤーを貼り付ける
    //view.layer = layer;
    view.wantsLayer = YES;
    [view.layer addSublayer:layer];

    surface->data.layer = layer;
  }

  return surface;
}

template <> ImpSurface::~ImpSurfaceTemplate<ImpSurfaceData>() {
  @autoreleasepool {
    if (this->data.layer != nil) {
      [this->data.layer removeFromSuperlayer];
      [this->data.layer release];
      this->data.layer = nil;
    }
    if (this->data.window != nullptr) {
      NSView *view = static_cast<ImpMacWindow *>(this->data.window)
                         ->getPlatformData()
                         .view;
      if (view != nil) {
        view.wantsLayer = NO;
      }
      this->data.window->release();
      this->data.window = nullptr;
    }
    if (this->data.device != nullptr) {
      // 参照カウントを減らす
      this->data.device->release();
      this->data.device = nullptr;
    }
  }
}

template <> ImpSurfaceData ImpSurface::getPlatformData() const {
  return this->data;
}
