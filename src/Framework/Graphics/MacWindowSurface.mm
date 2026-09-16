#include "../Application/ApplicationTemplate.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "GraphicsTemplate.hpp"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "MacGraphics.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Mac Window Surface
//
//  ========================================================

MacWindowSurface::MacWindowSurface(IGraphicsDevice *device, IWindow *window)
    : WindowSurface(device, window) {}

// ウィンドウ専用デストラクタ
MacWindowSurface::~MacWindowSurface() {
  @autoreleasepool {
    if (this->data.metal_layer != nil) {
      [this->data.metal_layer removeFromSuperlayer];
      [this->data.metal_layer release];
      this->data.metal_layer = nil;
    }
    if (this->window != nullptr) {
      NSView *view =
          static_cast<MacWindow *>(this->window)->getPlatformData().view;
      if (view != nil) {
        view.wantsLayer = NO;
      }
      // 親デストラクタに任せる
      // this->window->release();
      // this->window = nullptr;
    }
  }
}

MacWindowSurface *
MacWindowSurface::createMacSurfaceFromWindow(IGraphicsDevice *device,
                                             IWindow *window) {
  if (window == nullptr || device == nullptr) {
    return nullptr;
  }
  MacWindowSurface *surface =
      static_cast<MacWindowSurface *>(std::malloc(sizeof(MacWindowSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createWindowSurface)");
    return nullptr;
  }

  surface = new (surface) MacWindowSurface(device, window);
  // 参照カウントのインクリメントはコンストラクタにまかせる
  // surface->addRef();

  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  // surface->device = static_cast<MacGraphicsDevice *>(device);
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  // surface->device->addRef();

  // 参照カウントを増やす
  // surface->window = window;
  // window->addRef();

  @autoreleasepool {
    // getter を IWindow に追加すればいいがそれだと内部が漏れる
    WindowView *view =
        static_cast<MacWindow *>(surface->window)->getPlatformData().view;

    // view はポインタなので view に変更を加えると window
    // 側にも反映される
    // 型は NSView でも WindowView でもどっちでもいい
    if (view == nil) {
      surface->release();
      return nullptr;
    }

    CAMetalLayer *layer = [[CAMetalLayer alloc] init];
    // surface->addRef(); の直後ですでに device の参照カウントを増やしてある
    layer.device = static_cast<MacGraphicsDevice *>(surface->device)
                       ->getPlatformData()
                       .mtl_device;
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
      // 一応ガード用 if があるがこれが nil ということは createWindow
      // がおかしい
      [layer release];
      surface->release();
      return nullptr;
    }

    CGFloat scale = [view.window backingScaleFactor];
    if (scale == 0.0f) {
      surface->release();
      return nullptr;
    }
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(view.bounds.size.width * scale,
                                    view.bounds.size.height * scale);

    // Metal レイヤーを貼り付ける
    // view.layer = layer;
    view.wantsLayer = YES;
    [view.layer addSublayer:layer];

    surface->data.metal_layer = layer;

    // CAMetalView の drawable は 3 枚ある
    surface->data.in_flight_semaphore = dispatch_semaphore_create(3);
    if (surface->data.in_flight_semaphore == nil) {
      device->release();
    }
  }

  return surface;
}

//  ========================================================
//
//  Window Surface Template
//
//  ========================================================

template <>
bool WindowSurface::render(RenderCallBack callback, void *data,
                           const RenderPassDesc pass_desc) {
  bool result = false;
  @autoreleasepool {
    // チケットを消費
    // 残っていればスルー
    // 残っていなければ返却されるまで停止
    if (dispatch_semaphore_wait(this->data.in_flight_semaphore,
                                pass_desc.frame_dropping ==
                                        FrameDropping::Enable
                                    ? DISPATCH_TIME_NOW
                                    : DISPATCH_TIME_FOREVER) != 0) {
      return false;
    }

    MacWindow *window = static_cast<MacWindow *>(this->window);
    MacGraphicsDevice *device = static_cast<MacGraphicsDevice *>(this->device);
    CAMetalLayer *metal_layer = this->data.metal_layer;

    if (callback == nullptr || window == nullptr || device == nullptr ||
        metal_layer == nullptr) {
      return false;
    }

    NSView *view = window->getPlatformData().view;

    if (view == nil) {
      return false;
    }

    id<MTLRenderPipelineState> pipeline_state =
        device->getPlatformData().pipeline_state;

    id<MTLRenderPipelineState> pipeline_state_tex =
        device->getPlatformData().pipeline_state_tex;

    id<MTLRenderPipelineState> pipeline_state_tex_outline =
        device->getPlatformData().pipeline_state_tex_outline;

    id<MTLSamplerState> sampler_state = device->getPlatformData().sampler_state;

    if (pipeline_state == nil || pipeline_state_tex == nil ||
        pipeline_state_tex_outline == nil || sampler_state == nil) {
      return false;
    }

    // 解像度を設定 (drawaableSize だけ手動でサイズ変更が必要)
    CGSize size = metal_layer.bounds.size;
    CGFloat scale = metal_layer.contentsScale;
    CGSize new_drawble_size =
        CGSizeMake(size.width * scale, size.height * scale);
    if (!CGSizeEqualToSize(new_drawble_size, metal_layer.drawableSize)) {
      metal_layer.drawableSize = new_drawble_size;
    }

    // Retina ディスプレイの論理ポイント座標系を使用
    float true_width = size.width;
    float true_height = size.height;

    // バックバッファを取得
    // 複数のバッファがあって, 表示中のバッファ,
    // 描画中のバッファというようになってるらしい (ティアリング 防止)
    // 3 枚あるらしい
    // 全てのバッファが埋まっているとここでスレッドが一時停止する
    id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
    if (drawable == nil) {
      // チケットを返却
      dispatch_semaphore_signal(this->data.in_flight_semaphore);
      return false;
    }

    RenderHelper helper(device);
    MTLRenderPassDescriptor *mtl_pass_desc =
        helper.getMTLRenderPassDescripter(drawable.texture, &pass_desc);
    if (mtl_pass_desc == nil) {
      return false;
    }

    // begin
    id<MTLCommandBuffer> cmd_buffer =
        [device->getPlatformData().command_queue commandBuffer];
    if (cmd_buffer == nil) {
      return false;
    }
    id<MTLRenderCommandEncoder> encoder =
        [cmd_buffer renderCommandEncoderWithDescriptor:mtl_pass_desc];
    if (encoder == nil) {
      return false;
    }

    // 描画処理
    result = helper.renderBase(device, encoder, true_width,
                      true_height, callback, data);

    // end
    [encoder endEncoding];
    [cmd_buffer presentDrawable:drawable];

    // ローカル変数にコピーしないと this がキャプチャされる
    // キャプチャしたものはヒープにコピーされて retain される
    dispatch_semaphore_t semaphore = this->data.in_flight_semaphore;
    [cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
      dispatch_semaphore_signal(semaphore); // 返却
    }];

    [cmd_buffer commit];

    // ウィンドウサイズ変更中に端の方にウィンドウの地肌が出るのを防ぐ
    // GPU が描画を始めようとするまで待つので描画されない部分を減らせる
    // 全てのデリゲート・イベントはメインスレッド
    if (window->getPlatformData().resizing) {
      [cmd_buffer waitUntilScheduled];
    }

    return result;
  }
}
