#include "MacSurface.h"
#include "MacGraphicsDevice.h"
#include "MacRenderPass.h"
#include "MacTextureSurface.h"
#include "MacWindowSurface.h"

//  ========================================================
//
//  Common Mac Surface
//
//  ========================================================

// Render() の補助
RenderHelper::RenderHelper(IGraphicsDevice *device) {
  if (device == nullptr) {
    this->is_ready = false;
    return;
  }
  this->device = device;
  this->device->addRef();
  @autoreleasepool {
    this->mtl_pass_desc = [[MTLRenderPassDescriptor alloc] init];
    if (this->mtl_pass_desc == nil) {
      this->is_ready = false;
    }
  }
  this->is_ready = true;
}

RenderHelper::~RenderHelper() {
  @autoreleasepool {
    if (this->device != nullptr) {
      this->device->release();
      this->device = nullptr;
    }
    if (this->mtl_pass_desc != nil) {
      [this->mtl_pass_desc release];
    }
  }
  this->is_ready = false;
}

bool RenderHelper::isReady() const { return this->is_ready; }

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

bool RenderHelper::renderBase(id<MTLRenderCommandEncoder> encoder,
                              float true_width, float true_height,
                              RenderCallBack callback, void *data) {
  if (encoder == nil || device == nil || true_height == 0.0f ||
      true_height == 0.0f) {
    return false;
  }

  MacGraphicsDevice *mac_device =
      static_cast<MacGraphicsDevice *>(this->device);
  id<MTLRenderPipelineState> pipeline_state =
      mac_device->getPlatformData().pipeline_state;
  id<MTLRenderPipelineState> pipeline_state_tex =
      mac_device->getPlatformData().pipeline_state_tex;
  id<MTLRenderPipelineState> pipeline_state_tex_outline =
      mac_device->getPlatformData().pipeline_state_tex_outline;
  id<MTLSamplerState> sampler_state =
      mac_device->getPlatformData().sampler_state;

  if (pipeline_state == nil || pipeline_state_tex == nil ||
      pipeline_state_tex_outline == nil || sampler_state == nil) {
    return false;
  }

  // ここでコールバック
  MacRenderPass pass(device, encoder);
  if (pass.isReady()) {
    [encoder retain];
    callback(&pass, data);
    [encoder release];
  }
  return true;
}

//  ========================================================
//
//  Common Surface Template
//
//  ========================================================

template class SurfaceTemplate<WindowSurfaceData>;
template class SurfaceTemplate<TextureSurfaceData>;

// 共通デストラクタ
template <class PlatformData>
SurfaceTemplate<PlatformData>::~SurfaceTemplate<PlatformData>() {
  @autoreleasepool {
    if (this->data.in_flight_semaphore != nil) {
      dispatch_release(this->data.in_flight_semaphore);
      this->data.in_flight_semaphore = nil;
    }
    // ~CommonSurface に任せる
    // if (this->device != nullptr) {
    //   // 参照カウントを減らす
    //   this->device->release();
    //   this->device = nullptr;
    // }
  }
}

// 共通ゲッター
template <class PlatformData>
PlatformData SurfaceTemplate<PlatformData>::getPlatformData() const {
  return this->data;
}
