#pragma once
#include "../CommonGraphics.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

//  ========================================================
//
//  Surface
//
//  ========================================================

class MacSurface : public CommonSurface {
protected:
  dispatch_semaphore_t in_flight_semaphore = nil;

  MacSurface(IGraphicsDevice *device, IObject *window_or_texture)
      : CommonSurface(device, window_or_texture) {}

public:
  MacSurface() = delete;
  ~MacSurface();
};

// ヘルパークラス
class RenderHelper {
private:
  MTLRenderPassDescriptor *mtl_pass_desc = nil;
  IGraphicsDevice *device = nullptr;
  bool is_ready = false;

public:
  static constexpr float inv_255 = 1.0f / 255.0f;
  MTLRenderPassDescriptor *getMTLRenderPassDescripter(id<MTLTexture>,
                                                      const RenderPassDesc *);
  [[nodiscard]] bool renderBase(id<MTLRenderCommandEncoder>, float, float,
                                RenderCallBack callback, void *data);
  bool isReady() const;
  RenderHelper(IGraphicsDevice *);
  ~RenderHelper();
};
