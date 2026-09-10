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
//  Graphics Device
//
//  ========================================================

MacGraphicsDevice *MacGraphicsDevice::createMacGraphicsDevice(
    ImpApplication<ImpApplicationData> *appInstance) {
  @autoreleasepool {
    MacGraphicsDevice *device = static_cast<MacGraphicsDevice *>(
        std::malloc(sizeof(MacGraphicsDevice)));
    if (device == nullptr) {
      std::perror("malloc failed (createGraphicsDevice)");
      return nullptr;
    }

    device = new (device) MacGraphicsDevice;
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
    if (shader_data == nil) {
      device->release();
      return nullptr;
    }

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
    id<MTLFunction> vs = [library newFunctionWithName:@"vertexMain"];
    id<MTLFunction> ps = [library newFunctionWithName:@"fragmentMain"];
    id<MTLFunction> vs_tex = [library newFunctionWithName:@"vertexMainUV"];
    id<MTLFunction> ps_tex = [library newFunctionWithName:@"fragmentMainTex"];

    auto shaderRelease = [vs, ps, vs_tex, ps_tex]() -> void {
      if (vs != nil) {
        [vs release];
      }
      if (ps != nil) {
        [ps release];
      }
      if (vs_tex != nil) {
        [vs_tex release];
      }
      if (ps_tex != nil) {
        [ps_tex release];
      }
    };

    // vs と ps (tex も) が参照カウントを増やすので release する
    [library release];

    if (vs == nil || ps == nil || vs_tex == nil || ps_tex == nil) {
      std::cerr << "Failed to find vertex or fragment function" << std::endl;
      shaderRelease();
      device->release();
      return nullptr;
    }

    // パイプラインステートの設定
    MTLRenderPipelineDescriptor *pipeline_desc =
        [[MTLRenderPipelineDescriptor alloc] init];
    if (pipeline_desc == nil) {
      shaderRelease();
      device->release();
      return nullptr;
    }

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

    pipeline_desc.vertexFunction = vs_tex;
    pipeline_desc.fragmentFunction = ps_tex;

    device->data.pipeline_state_tex =
        [device->data.device newRenderPipelineStateWithDescriptor:pipeline_desc
                                                            error:&error];
    [pipeline_desc release];
    shaderRelease();
    if (device->data.pipeline_state == nil ||
        device->data.pipeline_state_tex == nil) {
      device->release();
      return nullptr;
    }

    MTLSamplerDescriptor *sampler_desc = [[MTLSamplerDescriptor alloc] init];
    if (sampler_desc == nil) {
      device->release();
      return nullptr;
    }

    sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
    sampler_desc.magFilter = MTLSamplerMinMagFilterNearest;
    sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;

    device->data.sampler_state =
        [device->data.device newSamplerStateWithDescriptor:sampler_desc];

    // 参照カウントが増えるので release しておく
    [sampler_desc release];

    if (device->data.sampler_state == nil) {
      device->release();
      return nullptr;
    }

    device->data.in_flight_semaphore = dispatch_semaphore_create(3);
    if (device->data.in_flight_semaphore == nil) {
      device->release();
      return nullptr;
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
ImpGraphicsDevice::~ImpGraphicsDeviceTemplate<ImpGraphicsDeviceData,
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
    if (this->data.sampler_state != nil) {
      [this->data.sampler_state release];
      this->data.sampler_state = nil;
    }
    if (this->data.pipeline_state_tex != nil) {
      [this->data.pipeline_state_tex release];
      this->data.pipeline_state_tex = nil;
    }
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

template <> ImpGraphicsDeviceData ImpGraphicsDevice::getPlatformData() const {
  return this->data;
}

template <>
bool ImpGraphicsDevice::render(ISurface *isurface, RenderCallBack callback,
                               void *data, const RenderPassDesc pass_desc) {
  if (isurface == nullptr || callback == nullptr) {
    return false;
  }

  bool result = true;
  @autoreleasepool {
    //[CATransaction begin];
    //[CATransaction setDisableActions:YES];

    ImpSurface *surface = static_cast<ImpSurface *>(isurface);
    CAMetalLayer *layer = surface->getPlatformData().layer;

    if (layer == nil) {
      return false;
    }

    ImpMacWindow *window =
        static_cast<ImpMacWindow *>(surface->getPlatformData().window);

    if (window == nullptr) {
      return false;
    }

    NSView *view = window->getPlatformData().view;

    if (view == nil) {
      return false;
    }

    // Retina ディスプレイの論理ポイント座標系を使用
    float true_width = 0;
    float true_height = 0;
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

      // 解像度を設定 (drawaableSize だけ手動でサイズ変更が必要)
      CAMetalLayer *metal_layer = surface->getPlatformData().layer;
      CGSize size = view.bounds.size;
      CGFloat scale = metal_layer.contentsScale;
      CGSize new_drawble_size =
          CGSizeMake(size.width * scale, size.height * scale);
      if (!CGSizeEqualToSize(new_drawble_size, metal_layer.drawableSize)) {
        metal_layer.drawableSize = new_drawble_size;
      }
          

      true_width = view.bounds.size.width;
      true_height = view.bounds.size.height;
    } else {
      // ウィンドウにバインドされてない場合
      // true_width = surface->getPlatformData().width;
      // true_height = surface->getPlatformData().height;
    }

    if (dispatch_semaphore_wait(this->data.in_flight_semaphore,
                                pass_desc.frame_dropping ==
                                        FrameDropping::Enable
                                    ? DISPATCH_TIME_NOW
                                    : DISPATCH_TIME_FOREVER) != 0) {
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
        MacRenderPass pass(encoder /*, this->data.vertex_buffer*/);
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

template <>
ITexture *ImpGraphicsDevice::createTexture(int width, int height,
                                           TextureDrawable drawable_flag) {
  return MacTexture::createMacTexture(this, width, height, drawable_flag);
}

template <>
ISurface *ImpGraphicsDevice::createSurfaceFromWindow(IWindow *window) {
  return MacSurface::createMacSurfaceFromWindow(
      static_cast<MacGraphicsDevice *>(this), window);
}

template <>
IGraphicsDevice *ImpApplication<ImpApplicationData>::createGraphicsDevice() {
  IGraphicsDevice *device = MacGraphicsDevice::createMacGraphicsDevice(this);
  return device;
}
