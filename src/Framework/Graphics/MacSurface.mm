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

// Render() の補助
RenderHelper::RenderHelper(MacGraphicsDevice *device) {
  @autoreleasepool {
    this->mtl_pass_desc = [[MTLRenderPassDescriptor alloc] init];
  }
}

RenderHelper::~RenderHelper() {
  @autoreleasepool {
    if (this->mtl_pass_desc != nil) {
      [this->mtl_pass_desc release];
    }
  }
}

MTLRenderPassDescriptor *
RenderHelper::getMTLRenderPassDescripter(id<MTLTexture> mtl_texture,
                                         const RenderPassDesc *pass_desc) {
  @autoreleasepool {
    if (this->mtl_pass_desc == nil || mtl_texture == nil ||
        pass_desc == nullptr) {
      return nil;
    }
    // 描き込み先のテクスチャ
    this->mtl_pass_desc.colorAttachments[0].texture = mtl_texture;
    // 描画を開始時に前のフレームをどうするか
    // Clear: 指定色でクリア, Load: 保持 (遅いらしい)
    this->mtl_pass_desc.colorAttachments[0].loadAction =
        pass_desc->clear ? MTLLoadActionClear : MTLLoadActionLoad;

    // 描画が終わった後, 結果をテクスチャに保存するか
    this->mtl_pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;

    // クリアに使う色
    this->mtl_pass_desc.colorAttachments[0].clearColor =
        MTLClearColorMake(((pass_desc->color >> 16) & 0xFF) * inv_255,
                          ((pass_desc->color >> 8) & 0xFF) * inv_255,
                          (pass_desc->color & 0xFF) * inv_255,
                          ((pass_desc->color >> 24) & 0xFF) * inv_255);
    return this->mtl_pass_desc;
  }
}

void RenderHelper::renderBase(id<MTLRenderCommandEncoder> encoder,
                              id<MTLRenderPipelineState> pipeline_state,
                              float true_width, float true_height,
                              RenderCallBack callback, void *data) {
  if (encoder == nil || pipeline_state == nil || true_width == 0.0f ||
      true_height == 0.0f) {
    return;
  }
  [encoder setRenderPipelineState:pipeline_state];

  struct {
    float width;
    float r_height;
    float inv_255;
  } viewport = {true_width, 2.0f / true_height, inv_255};

  [encoder setVertexBytes:&viewport length:sizeof(viewport) atIndex:1];

  // ここでコールバック
  MacRenderPass pass(encoder /*, this->data.vertex_buffer*/);
  [encoder retain];
  callback(&pass, data);
  [encoder release];
}

// 共通デストラクタ
template <class PlatformData>
ImpSurfaceTemplate<PlatformData>::~ImpSurfaceTemplate<PlatformData>() {
  @autoreleasepool {
    if (this->data.in_flight_semaphore != nil) {
      dispatch_release(this->data.in_flight_semaphore);
      this->data.in_flight_semaphore = nil;
    }
    if (this->data.device != nullptr) {
      // 参照カウントを減らす
      this->data.device->release();
      this->data.device = nullptr;
    }
  }
}

// 共通ゲッター
template <class PlatformData>
PlatformData ImpSurfaceTemplate<PlatformData>::getPlatformData() const {
  return this->data;
}

// ウィンドウ専用デストラクタ
MacWindowSurface::~MacWindowSurface() {
  @autoreleasepool {
    if (this->data.metal_layer != nil) {
      [this->data.metal_layer removeFromSuperlayer];
      [this->data.metal_layer release];
      this->data.metal_layer = nil;
    }
    if (this->data.window != nullptr) {
      NSView *view = static_cast<MacWindow *>(this->data.window)
                         ->getPlatformData()
                         .view;
      if (view != nil) {
        view.wantsLayer = NO;
      }
      this->data.window->release();
      this->data.window = nullptr;
    }
  }
}

template <>
bool ImpWindowSurface::render(RenderCallBack callback, void *data,
                              const RenderPassDesc pass_desc) {
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

    MacWindow *window = static_cast<MacWindow *>(this->data.window);
    MacGraphicsDevice *device = this->data.device;
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

    if (pipeline_state == nil) {
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
        [this->data.device->getPlatformData().command_queue commandBuffer];
    if (cmd_buffer == nil) {
      return false;
    }
    id<MTLRenderCommandEncoder> encoder =
        [cmd_buffer renderCommandEncoderWithDescriptor:mtl_pass_desc];
    if (encoder == nil) {
      return false;
    }

    // 描画処理
    helper.renderBase(encoder, pipeline_state, true_width, true_height,
                      callback, data);

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

    return true;
  }
}

MacWindowSurface *
MacWindowSurface::createMacSurfaceFromWindow(MacGraphicsDevice *device,
                                             IWindow *window) {
  if (window == nullptr) {
    return nullptr;
  }
  MacWindowSurface *surface =
      static_cast<MacWindowSurface *>(std::malloc(sizeof(MacWindowSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createWindowSurface)");
    return nullptr;
  }

  surface = new (surface) MacWindowSurface;
  surface->addRef();

  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  surface->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  surface->data.device->addRef();

  // 参照カウントを増やす
  surface->data.window = window;
  window->addRef();

  @autoreleasepool {
    // getter を IWindow に追加すればいいがそれだと内部が漏れる
    WindowView *view = static_cast<MacWindow *>(surface->data.window)
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

// テクスチャ専用デストラクタd
MacTextureSurface::~MacTextureSurface() {
  @autoreleasepool {
    if (this->data.texture != nullptr) {
      this->data.texture->release();
    }
  }
}

MacTextureSurface *
MacTextureSurface::createMacSurfaceFromTexture(MacGraphicsDevice *device,
                                               ITexture *texture) {
  if (texture == nullptr) {
    return nullptr;
  }
  MacTextureSurface *surface =
      static_cast<MacTextureSurface *>(std::malloc(sizeof(MacTextureSurface)));
  if (surface == nullptr) {
    std::perror("malloc failed (createTextureSurface)");
    return nullptr;
  }

  surface = new (surface) MacTextureSurface;
  surface->addRef();

  // この中では Objc のオブジェクトに対して操作してないから
  // autoreleasepool はいらん
  // device を参照 (直接 MTLDevice を代入するのでなく MacGraphicsDevice
  // だからプールはいらん)
  surface->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  surface->data.device->addRef();

  // 参照カウントを増やす
  surface->data.texture = texture;
  texture->addRef();

  @autoreleasepool {
    // Texture は 1 枚の描画先なので処理中か否かの二パターンの状態がある
    surface->data.in_flight_semaphore = dispatch_semaphore_create(2);
    if (surface->data.in_flight_semaphore == nil) {
      device->release();
    }
  }

  return surface;
}

template <>
bool ImpTextureSurface::render(RenderCallBack callback, void *data,
                               const RenderPassDesc pass_desc) {
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

    MacTexture *texture = static_cast<MacTexture *>(this->data.texture);
    MacGraphicsDevice *device =
        static_cast<MacGraphicsDevice *>(this->data.device);
    if (texture == nullptr || device == nullptr) {
      return false;
    }

    id<MTLTexture> mtl_texture = texture->getPlatformData().texture;
    id<MTLRenderPipelineState> pipeline_state =
        device->getPlatformData().pipeline_state_tex;

    float true_width = texture->getPlatformData().width;
    float true_height = texture->getPlatformData().height;

    if (mtl_texture == nil || pipeline_state == nil || true_width == 0 ||
        true_height == 0) {
      return false;
    }

    RenderHelper helper(device);
    MTLRenderPassDescriptor *mtl_pass_desc =
        helper.getMTLRenderPassDescripter(mtl_texture, &pass_desc);

    // begin
    id<MTLCommandBuffer> cmd_buffer =
        [this->data.device->getPlatformData().command_queue commandBuffer];
    if (cmd_buffer == nil) {
      return false;
    }
    id<MTLRenderCommandEncoder> encoder =
        [cmd_buffer renderCommandEncoderWithDescriptor:mtl_pass_desc];
    if (encoder == nil) {
      return false;
    }

    helper.renderBase(encoder, pipeline_state, true_width, true_height,
                      callback, data);

    // end
    [encoder endEncoding];

    // ローカル変数にコピーしないと this がキャプチャされる
    // キャプチャしたものはヒープにコピーされて retain される
    dispatch_semaphore_t semaphore = this->data.in_flight_semaphore;
    [cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
      dispatch_semaphore_signal(semaphore); // 返却
    }];

    [cmd_buffer commit];

    return true;
  }
}
