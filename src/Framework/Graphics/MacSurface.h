#pragma once

#include "SurfaceTemplate.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

//  ========================================================
//
//  Surface
//
//  ========================================================

struct SurfaceData {
  dispatch_semaphore_t in_flight_semaphore = nil;
};

// ヘルパークラス
class RenderHelper {
private:
  static constexpr float inv_255 = 1.0f / 255.0f;
  MTLRenderPassDescriptor *mtl_pass_desc = nil;
  IGraphicsDevice *device = nullptr;
  bool is_ready = false;

public:
  MTLRenderPassDescriptor *getMTLRenderPassDescripter(id<MTLTexture>,
                                                      const RenderPassDesc *);
  [[nodiscard]] bool renderBase(id<MTLRenderCommandEncoder>, float, float,
                                RenderCallBack callback, void *data);
  bool isReady() const;
  RenderHelper(IGraphicsDevice *);
  ~RenderHelper();
};