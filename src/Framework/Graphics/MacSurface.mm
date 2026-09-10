#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
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
//  Surface
//
//  ========================================================

MacSurface *MacSurface::createMacSurfaceBase(MacGraphicsDevice *device) {
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

MacSurface *MacSurface::createMacSurfaceFromWindow(MacGraphicsDevice *device,
                                                   IWindow *window) {
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
    WindowView *view = static_cast<ImpMacWindow *>(surface->data.window)
                           ->getPlatformData()
                           .view;

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
    // view.layer = layer;
    view.wantsLayer = YES;
    [view.layer addSublayer:layer];

    surface->data.layer = layer;

    // CAMetalView の drawable は 3 枚ある
    surface->data.in_flight_semaphore = dispatch_semaphore_create(3);
    if (surface->data.in_flight_semaphore == nil) {
      device->release();
    }
  }

  return surface;
}

template <>
bool ImpSurface::render(RenderCallBack callback, void *data,
                        const RenderPassDesc pass_desc) {
  @autoreleasepool {
    if (callback == nullptr || this->data.device == nullptr) {
      return false;
    }
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

    constexpr float inv_255 = 1.0f / 255.0f;
    auto setMTLRenderPassDesc =
        [pass_desc](
            id<MTLTexture> drawable_texture) -> MTLRenderPassDescriptor * {
      MTLRenderPassDescriptor *desc = [[MTLRenderPassDescriptor alloc] init];
      if (desc == nil) {
        return nil;
      }
      // 描き込み先のテクスチャ
      desc.colorAttachments[0].texture = drawable_texture;

      // 描画を開始時に前のフレームをどうするか
      // Clear: 指定色でクリア, Load: 保持 (遅いらしい)
      desc.colorAttachments[0].loadAction =
          pass_desc.clear ? MTLLoadActionClear : MTLLoadActionLoad;

      // 描画が終わった後, 結果をテクスチャに保存するか
      desc.colorAttachments[0].storeAction = MTLStoreActionStore;

      // クリアに使う色
      desc.colorAttachments[0].clearColor =
          MTLClearColorMake(((pass_desc.color >> 16) & 0xFF) * inv_255,
                            ((pass_desc.color >> 8) & 0xFF) * inv_255,
                            (pass_desc.color & 0xFF) * inv_255,
                            ((pass_desc.color >> 24) & 0xFF) * inv_255);
      return desc;
    };

    float true_width = 0.0f;
    float true_height = 0.0f;

    id<MTLCommandQueue> cmd_queue =
        this->data.device->getPlatformData().command_queue;
    if (cmd_queue == nil) {
      return false;
    }

    MTLRenderPassDescriptor *desc = nil;
    id<MTLRenderPipelineState> pipeline_state = nil;
    id<CAMetalDrawable> drawable = nil;

    // nil じゃなければウィンドウ
    if (this->data.layer != nil) {
      // 使うパイプラインステートを保存
      pipeline_state = this->data.device->getPlatformData().pipeline_state;
      if (pipeline_state == nil) {
        return false;
      }
      CAMetalLayer *metal_layer = this->data.layer;
      // 解像度を設定 (drawaableSize だけ手動でサイズ変更が必要)
      CGSize size = metal_layer.bounds.size;
      CGFloat scale = metal_layer.contentsScale;
      CGSize new_drawble_size =
          CGSizeMake(size.width * scale, size.height * scale);
      if (!CGSizeEqualToSize(new_drawble_size, metal_layer.drawableSize)) {
        metal_layer.drawableSize = new_drawble_size;
      }

      true_width = metal_layer.bounds.size.width;
      true_height = metal_layer.bounds.size.height;

      // バックバッファを取得
      // 複数のバッファがあって, 表示中のバッファ,
      // 描画中のバッファというようになってるらしい (ティアリング 防止)
      // 3 枚あるらしい
      // 全てのバッファが埋まっているとここでスレッドが一時停止する
      drawable = [metal_layer nextDrawable];
      if (drawable == nil) {
        // チケット返却
        dispatch_semaphore_signal(this->data.in_flight_semaphore);
        return false;
      }
      desc = setMTLRenderPassDesc(drawable.texture);
      if (desc == nil) {
        return false;
      }
    } else {
      // nil だったらテクスチャ
      id<MTLTexture> mtl_texture = static_cast<MacTexture *>(this->data.texture)
                                       ->getPlatformData()
                                       .texture;
      // 使うパイプラインステートを保存
      pipeline_state = this->data.device->getPlatformData().pipeline_state;
      if (mtl_texture == nil || pipeline_state == nil) {
        return false;
      }
      desc = setMTLRenderPassDesc(mtl_texture);
      if (desc == nil) {
        return false;
      }
    }

    // begin
    id<MTLCommandBuffer> cmdBuffer = [cmd_queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [cmdBuffer renderCommandEncoderWithDescriptor:desc];
    [desc release];
    // パイプラインステートをセット
    if (pipeline_state != nil) {
      [encoder setRenderPipelineState:pipeline_state];

      struct {
        float width;
        float r_height;
        float inv_255;
      } viewport = {true_width, 2.0f / true_height, inv_255};

      [encoder setVertexBytes:&viewport length:sizeof(viewport) atIndex:1];

      // ここでコールバック (beign-end)
      MacRenderPass pass(encoder /*, this->data.vertex_buffer*/);
      [encoder retain];
      // ! = nullptr 保証済み
      callback(&pass, data);
      [encoder release];
    }
    // end
    [encoder endEncoding];
    
    // ローカル変数にコピーしないと this がキャプチャされる
    // キャプチャしたものはヒープにコピーされて retain される
    dispatch_semaphore_t semaphore = this->data.in_flight_semaphore;
    [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
      dispatch_semaphore_signal(semaphore); // 返却
    }];

    // nil でなければウィンドウ
    if (this->data.layer != nil && drawable != nil) {
      [cmdBuffer presentDrawable:drawable];
      [cmdBuffer commit];
      // ウィンドウサイズ変更中に端の方にウィンドウの地肌が出るのを防ぐ
      // GPU が描画を始めようとするまで待つので描画されない部分を減らせる
      // 全てのデリゲート・イベントはメインスレッド
      ImpMacWindow *window = static_cast<ImpMacWindow *>(this->data.window);
      if (window != nullptr && window->getPlatformData().resizing) {
        [cmdBuffer waitUntilScheduled];
      }
    } else {
      [cmdBuffer commit];
    }
    return true;
  }
}

template <> ImpSurface::~ImpSurfaceTemplate<ImpSurfaceData>() {
  @autoreleasepool {
    if (this->data.in_flight_semaphore != nil) {
      dispatch_release(this->data.in_flight_semaphore);
      this->data.in_flight_semaphore = nil;
    }
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
