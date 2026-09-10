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
ITexture *ImpGraphicsDevice::createTexture(int width, int height,
                                           TextureDrawable drawable_flag) {
  return MacTexture::createMacTexture(static_cast<MacGraphicsDevice *>(this),
                                      width, height, drawable_flag);
}

template <>
ISurface *ImpGraphicsDevice::createSurfaceFromWindow(IWindow *window) {
  return MacWindowSurface::createMacSurfaceFromWindow(
      static_cast<MacGraphicsDevice *>(this), window);
}

template <> ISurface *ImpGraphicsDevice::createSurfaceFromTexture(ITexture *) {
  return nullptr;
}

template <>
IGraphicsDevice *ImpApplication<ImpApplicationData>::createGraphicsDevice() {
  IGraphicsDevice *device = MacGraphicsDevice::createMacGraphicsDevice(this);
  return device;
}
