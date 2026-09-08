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

//  ----------------------------
//  Render Pass
//  ----------------------------
class ImpMacRenderPass : public ImpRenderPass {
public:
  ImpMacRenderPass(id<MTLRenderCommandEncoder> /*, id<MTLBuffer>*/);
};

ImpMacRenderPass::ImpMacRenderPass(id<MTLRenderCommandEncoder> encoder/*,
                                   id<MTLBuffer> vertex_buf*/) {
  this->data.encoder = encoder;
  // this->data.vertex_buffer = vertex_buf;
}

template <>
bool ImpRenderPass::drawVertices(const Vertex *vertices, int vertex_count) {
  // render() 内でしか呼ばれない, 呼び出し元で既に @autoreleasepool してる
  // そもそもここで使ってるメソッドはリソース生成しないらしい
  if (this->data.encoder == nil /*|| this->data.vertex_buffer == nil*/ ||
      vertices == nil || vertex_count <= 0) {
    return false;
  }

  // void *ptr = [this->data.vertex_buffer contents];
  // std::memcpy(ptr, vertices, sizeof(Vertex) * vertex_count);

  // スロット 0 に頂点データ をセット(4KB
  // 以内ならバッファを使わなくていいらしい)
  [this->data.encoder setVertexBytes:vertices
                              length:sizeof(Vertex) * vertex_count
                             atIndex:0];

  // スロット 0 に頂点バッファをセット
  //[this->data.encoder setVertexBuffer:this->data.vertex_buffer
  //                             offset:0
  //                            atIndex:0];

  [this->data.encoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:0
                         vertexCount:vertex_count];

  return true;
}

//  ----------------------------
//  Texture
//  ----------------------------
class ImpMacTexture : public ImpTexture {
public:
};

//  ----------------------------
//  Surface
//  ----------------------------

// プラットフォーム依存内部用クラス
// コンストラクタで MTLDeivce を渡すために経由する
// 渡し忘れを防ぐため, ImpSurface のコンストラクタが private になっている
class ImpMacSurface : public ImpSurface {
public:
  ImpMacSurface(ImpApplication<ImpApplicationData> *, ImpMacGraphicsDevice *);
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
                             ImpMacGraphicsDevice *device) {
  // device を参照
  this->data.device = device;
  // こいつの参照が 0 にならないと appInstance は解放できない仕様
  this->data.device->addRef();
}

//  ----------------------------
//  Graphics Device
//  ----------------------------

template <>
ImpMacGraphicsDevice::~ImpGraphicsDevice<ImpGraphicsDeviceData,
                                         ImpApplicationData>() {
  @autoreleasepool {
    if (this->data.in_flight_semaphore != nil) {
      dispatch_release(this->data.in_flight_semaphore);
      this->data.in_flight_semaphore = nil;
    }
    // if (this->data.vertex_buffer != nil) {
    //   [this->data.vertex_buffer release];
    //   this->data.vertex_buffer = nil;
    // }
    if (this->data.pipeline_state != nil) {
      [this->data.pipeline_state release];
      this->data.pipeline_state = nil;
    }
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

  bool result = true;
  @autoreleasepool {
    //[CATransaction begin];
    //[CATransaction setDisableActions:YES];

    if (dispatch_semaphore_wait(this->data.in_flight_semaphore,
                                pass_desc.frame_dropping ==
                                        FrameDropping::Enable
                                    ? DISPATCH_TIME_NOW
                                    : DISPATCH_TIME_FOREVER) != 0) {
      return false;
    }

    ImpSurface *surface = static_cast<ImpSurface *>(isurface);
    CAMetalLayer *layer = surface->getPlatformData().layer;

    if (layer == nil) {
      return false;
    }

    // バックバッファを取得
    // 複数のバッファがあって, 表示中のバッファ,
    // 描画中のバッファというようになってるらしい (ティアリング 防止)
    // 3 枚あるらしい
    // 全てのバッファが埋まっているとここでスレッドが一時停止する
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (drawable == nil) {
      dispatch_semaphore_signal(this->data.in_flight_semaphore);
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

    // Retina ディスプレイの論理ポイント座標系を使用
    float true_width = 0;
    float true_height = 0;
    ImpMacWindow *window =
        static_cast<ImpMacWindow *>(surface->getPlatformData().window);
    if (window != nullptr) {
      // ウィンドウにバインドされている場合
      // アンバインドした後に別の Surface
      // 上に描画したら最後の貼り付けていたウィンドウのサイズの比に変化する
      // ウィンドウに貼り付けている場合, Surface
      // の大きさを変えずにそのまま引き延ばすため
      // しかし, Metal では座標が正規化されているので解像度自体は直接扱わない
      // なかなか言語化が難しい
      // ウィンドウサイズに合わせて勝手に Surface
      // のサイズを変えていいならこの問題は起きない
      NSView *view =
          static_cast<ImpMacWindow *>(surface->getPlatformData().window)
              ->getPlatformData()
              .view;
      true_width = view.bounds.size.width;
      true_height = view.bounds.size.height;
    } else {
      // ウィンドウにバインドされてない場合
      // true_width = surface->getPlatformData().width;
      // true_height = surface->getPlatformData().height;
    }

    // パイプラインステートをセット
    if (this->data.pipeline_state != nil) {
      [encoder setRenderPipelineState:this->data.pipeline_state];

      constexpr float inv_255 = 1.0f / 255.0f;
      struct {
        float width;
        float r_height;
        float inv_255;
      } viewport = {true_width, 2.0f / true_height, inv_255};

      [encoder setVertexBytes:&viewport length:sizeof(viewport) atIndex:1];

      // ここでコールバック (beign-end)
      if (callback != nullptr) {
        ImpMacRenderPass pass(encoder /*, this->data.vertex_buffer*/);
        [encoder retain];
        callback(&pass, data);
        [encoder release];
      }
    } else {
      result = false;
    }

    // end
    [encoder endEncoding];
    [cmdBuffer presentDrawable:drawable];

    // ローカル変数にコピーしないと this がキャプチャされる
    // キャプチャしたものはヒープにコピーされて retain される
    dispatch_semaphore_t semaphore = this->data.in_flight_semaphore;
    [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
      dispatch_semaphore_signal(semaphore); // 返却
    }];
    [cmdBuffer commit];

    // ウィンドウサイズ変更中に端の方にウィンドウの地肌が出るのを防ぐ
    // GPU が描画を始めようとするまで待つので描画されない部分を減らせる
    // 全てのデリゲート・イベントはメインスレッド
    if (window != nullptr && window->getPlatformData().resizing) {
      [cmdBuffer waitUntilScheduled];
    }

    //[CATransaction commit];

    return result;
  }
}

template <> ITexture *ImpMacGraphicsDevice::createTexture(int, int) {
  return nullptr;
}

template <> ISurface *ImpMacGraphicsDevice::createSurface() {
  @autoreleasepool {
    ImpMacSurface *surface =
        static_cast<ImpMacSurface *>(std::malloc(sizeof(ImpMacSurface)));
    if (surface == nullptr) {
      std::perror("malloc failed (createSurface)");
      return nullptr;
    }

    surface = new (surface) ImpMacSurface(this->appInstance, this);
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

    dispatch_data_t shader_data =
        dispatch_data_create(shaders_metallib, shaders_metallib_size, nil,
                             ^{
                             });
    NSError *error = nil;
    id<MTLLibrary> library = [device->data.device newLibraryWithData:shader_data
                                                               error:&error];

    // library が参照カウントを増やすので release する
    dispatch_release(shader_data);

    if (library == nil) {
      std::cerr << "Failed to load metallib: "
                << (error ? [[error localizedDescription] UTF8String] : "")
                << std::endl;
      device->release();
      return nullptr;
    }

    // 関数名でシェーダーを取り出す
    id<MTLFunction> vs = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> ps = [library newFunctionWithName:@"fragment_main"];

    // vs と ps が参照カウントを増やすので release する
    [library release];

    if (vs == nil || ps == nil) {
      std::cerr << "Failed to find vertex or fragment function" << std::endl;
      if (vs != nil) {
        [vs release];
      }
      if (ps != nil) {
        [ps release];
      }
      device->release();
      return nullptr;
    }

    // パイプラインステートの設定
    MTLRenderPipelineDescriptor *pipeline_desc =
        [[MTLRenderPipelineDescriptor alloc] init];
    pipeline_desc.vertexFunction = vs;
    pipeline_desc.fragmentFunction = ps;
    pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    // アルファブレンドを行うかどうか
    pipeline_desc.colorAttachments[0].blendingEnabled = YES;
    // RGB (色) のブレンド式 (ガラスのように透ける表現にする場合):
    // Source * SourceAlpha + Dest * (1 - SourceAlpha)

    // 他にも Dest の係数を MTLBlendFactorOne (そのまま)
    // にすると色が重なるほど明るくなる

    // RGB のブレンドでは単一の図形の半透明にするやつで Alpha
    // のブレンドは二つの図形を重ねた時に
    // (特に両方とも半透明)
    // だった時に重なった部分の透明度がどうなるかだと思われる
    // 例: 透明度 50% の赤と透明度 50% の青
    // 色自体はブレンドされて紫
    // 重なりあった部分の透明度→Alpha のブレンドによって決まる
    // RGB のブレンド式と Alpha のブレンド式は独立しており, 別物

    // RGB をどう混ぜるか
    // Source の係数 (SourceAlpha の値) 与えられた透明度をそのまま使う
    pipeline_desc.colorAttachments[0].sourceRGBBlendFactor =
        MTLBlendFactorSourceAlpha;
    // Dest の係数: 1 - sourceAlpha
    pipeline_desc.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    // ブレンドの計算方法: 加算 +
    pipeline_desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;

    // A をどう混ぜるか
    // Alpha = Source * Factor + Dest * (1 - Factor)
    // Alpha (透明度) 自体の計算式
    // Source の係数: 係数は1でそのまま使うので MTLBlendFactorOne
    pipeline_desc.colorAttachments[0].sourceAlphaBlendFactor =
        MTLBlendFactorOne;
    // Dest の係数: 1 - Factor
    pipeline_desc.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    // ブレンドの計算方法: 加算 +
    pipeline_desc.colorAttachments[0].alphaBlendOperation =
        MTLBlendOperationAdd;

    device->data.pipeline_state =
        [device->data.device newRenderPipelineStateWithDescriptor:pipeline_desc
                                                            error:&error];

    // 参照カウントが増えるので release しておく
    [vs release];
    [ps release];
    [pipeline_desc release];

    if (device->data.pipeline_state == nil) {
      std::cerr << "Failed to create pipeline state: "
                << (error ? [[error localizedDescription] UTF8String] : "")
                << std::endl;
      device->release();
      return nullptr;
    }

    device->data.in_flight_semaphore = dispatch_semaphore_create(3);
    if (device->data.in_flight_semaphore == nil) {
      device->release();
    }

    // 頂点 1024 個分くらいのメモリをあらかじめ確保しておく
    // size_t buffer_size = sizeof(Vertex) * 1024;
    // device->data.vertex_buffer =
    //     [device->data.device newBufferWithLength:buffer_size
    //                                      options:MTLResourceStorageModeShared];
    // if (device->data.vertex_buffer == nil) {
    //   device->release();
    //   return nullptr;
    // }

    return device;
  }
}

template <>
IGraphicsDevice *ImpApplication<ImpApplicationData>::createGraphicsDevice() {
  ImpMacGraphicsDevice *device =
      ImpMacGraphicsDevice::createGraphicsDevice(this);
  return device;
}
